import os
import time
import subprocess
import getopt
import argparse
import re

FIO_IOPS_POS = 7
FIO_SLAT_POS_START = 9
FIO_CLAT_POS_START = 13
FIO_LAT_POS_START = 37

COLUMNS_MIN = ("iotype;bs;njobs;iodepth;iops;slatmin;slatmax;slatavg;clatmin;"
                                        "clatmax;clatavg;latmin;latmax;latavg")

# Default fio parameters (modifiable from input arguments)
fio_ioengine = 'libaio'
fio_num_jobs = '1'
fio_size = '1G'
fio_io_depth = '1'
fio_block_size = '4k'
fio_runtime = '60'
fio_exe = 'fio'
fio_rw = 'write'
fio_force = ''

# LightNVM configuration (modifiable from input arguments)
lnvm_device = 'sanity'
lnvm_config = '/sys/module/lnvm/parameters/configure_debug'
lnvm_file = ''
lnvm_config_cmd = ''
lnvm_remove_cmd = ''

# Test parameters (modifiable from input arguments)
n_iterations = 1
fio_test_dir = 'fio_tests/'

def initialize_file():
    kernel_version = os.uname()[2]

    f = open(kernel_version + time.strftime("%H%M%S") + "-fio.csv", "w+")
    f.write(COLUMNS_MIN+"\n")
    return f


def store_output(f, result):
    f.write(result + "\n")
    f.flush()


def execute_test(command):
        os.system("sleep 2") #Give time to finish inflight IOs
        sudo_command = "sudo sh -c '" + command + "'"
        output = subprocess.check_output(sudo_command, shell=True)
        return output


def execute_minimal_test(command):
    fio_type_offset = 0
    iops = 0.0
    slat = [0.0 for i in range(3)]
    clat = [0.0 for i in range(3)]
    lat = [0.0 for i in range(3)]

    result = ("" + str(fio_rw) + ";" + str(fio_block_size)
                + ";" + str(fio_num_jobs) + ";" + str(fio_io_depth) + ";")

    for i in range (0, n_iterations):
        output = execute_test(command)
        if "write" in fio_rw:
            fio_type_offset=41

        # fio must be called with --group_reporting. This means that all
        # statistics are grouped when using different jobs.

        # iops
        iops = iops + float(output.split(";")[fio_type_offset + FIO_IOPS_POS])

        # slat
        for j in range (0, 3):
            slat[j] = slat[j] + float(output.split(";")
                                        [fio_type_offset+FIO_SLAT_POS_START+j])
        # clat
        for j in range (0, 3):
            clat[j] = clat[j] + float(output.split(";")
                                        [fio_type_offset+FIO_CLAT_POS_START+j])
        # lat
        for j in range (0, 3):
            lat[j] = lat[j] + float(output.split(";")
                                        [fio_type_offset+FIO_LAT_POS_START+j])

    # iops
    result = result+str(iops / n_iterations)

    # slat
    for i in range (0, 3):
        result = result+";"+str(slat[i] / n_iterations)
    # clat
    for i in range (0, 3):
        result = result+";"+str(clat[i] / n_iterations)
    # lat
    for i in range (0, 3):
        result = result+";"+str(lat[i] / n_iterations)

    return result

def mm_tests(args, f):

    # Pull liblightnvm last version
    try:
        cmd = ("git submodule foreach git pull origin master")
        subprocess.call([cmd], stdout=None)
    except:
        print "IMPORTANT: Cannot update liblightnvm - media manager tests might be outdated"

    # install liblightnvm library from submodule
    result = subprocess.call(["sudo", "make", "-C", "../liblightnvm", "install_local"],
            stdout=None)

    result = subprocess.call(["sudo", "make", "-C", "../liblightnvm", "install"],
            stdout=None)

    # exuecute liblightnvm sanity checks
    result = subprocess.call(["sudo", "make", "-C", "../liblightnvm", "check"],
            stdout=None)

def generated(args, f):
    global fio_num_jobs
    global fio_io_depth
    global fio_block_size
    global fio_rw

    fio_exe_l = fio_exe
    if args.minimal:
       fio_exe_l = fio_exe_l + ' --minimal'
    fio_template = fio_test_dir + 'sanity.fio'

    print COLUMNS_MIN
    for fio_rw in ('write', 'randwrite', 'read', 'randread'):
        for fio_block_size in ('4k', '16k', '128k', '512k'):
            for fio_num_jobs in ('1', '32', '64'):
                for fio_io_depth in ('1', '8', '32', '64', '128'):
                    command = (" FILENAME=" + lnvm_file + " RW=" + fio_rw +
                            " SIZE=" + fio_size + " NUM_JOBS=" + fio_num_jobs +
                            " IO_DEPTH=" + fio_io_depth +
                            " BLOCK_SIZE=" + fio_block_size +
                            " RUNTIME=" + fio_runtime +
                            " IOENGINE=" + fio_ioengine + " " + fio_exe_l + " "
                            + fio_template)

                    if args.minimal:
                        result = execute_minimal_test(command)
                    else:
                        result = execute_test(command)
                    print result
                    if args.output:
                        store_output(f, result)


def custom(args):
    #TODO
    return 0


def scripts(args, f):
    global fio_num_jobs
    global fio_io_depth
    global fio_block_size
    global fio_rw

    fio_exe_l = fio_exe
    if args.minimal:
       fio_exe_l = fio_exe_l + ' --minimal'

    print COLUMNS_MIN
    for file in os.listdir(fio_test_dir):
        if file.endswith(".fio") and file !=  "sanity.fio":
            command = (fio_exe_l + ' ' + fio_test_dir + file)
            if args.minimal:
                fio_file = open(fio_test_dir + file, "r+")
                # Parse file and fill put fio parameters
                for line in fio_file.readlines():
                    if re.search('^rw=', line, re.I):
                        fio_rw = (line.split("=")[1]).strip()
                    if re.search('^bs=', line, re.I):
                        fio_block_size = (line.split("=")[1]).strip()
                    if re.search('^numjobs=', line, re.I):
                        fio_num_jobs = (line.split("=")[1]).strip()
                    if re.search('^iodepth=', line, re.I):
                        fio_io_depth = (line.split("=")[1]).strip()

                result = execute_minimal_test(command)
            else:
                result = execute_test(command)
                print ">>> Execute fio script:" + fio_test_dir + file

            print result
            if args.output:
                store_output(f, result)


def all(args, f):
    scripts(args, f)
    generated(args, f)

def configure_paths(lnvm_driver, args):
    global lnvm_file
    global lnvm_config_cmd
    global lnvm_remove_cmd

    lnvm_file = '/dev/' + lnvm_device
    lnvm_config = '/sys/module/lnvm/parameters/configure_debug'

    # liblightnvm creates the necessary targets for its unit tests
    if str(args).find("mm_tests") == -1:
        lnvm_target = "rrpc"
        lnvm_config_cmd = ("sudo sh -c 'echo \"a " + lnvm_driver + " " +
            lnvm_device + " " + lnvm_target + " 0:0\" > " + lnvm_config + "'")
        lnvm_remove_cmd = ("sudo sh -c 'echo \"d " + lnvm_device + "\" > " +
            lnvm_config + "'")

def main():
    parser = argparse.ArgumentParser(
        description=
        'Test LightNVM-enabled devices using flexible I/O tester (fio). \
        One of [-c, -s, -a] must be provided. A device [-d] must be also \
        provided.')

    parser.add_argument('-d', '--device', dest='device', action='store',
                        help='Choose device to create LightNVM target from. \
                        Supported devices can be seeing executing: \
                        \'cat /sys/module/lnvm/parameters/configure_debug\'. \
                        This argument is mandatory.')

    parser.add_argument('-mm', '--media-manager', dest='action',
            action='store_const', const=mm_tests, help='Execute LightNVM media \
            manager unit tests using liblightnvm.')

    parser.add_argument('-g', '--generated', dest='action', action='store_const',
                        const=generated, help='Execute generated fio tests \
                                (--minimal enabled).')

    parser.add_argument('-c', '--custom', dest='action', action='store_const',
                        const=custom, help='Execute custom fio tests.')

    parser.add_argument('-s', '--scripts', dest='action', action='store_const',
                        const=scripts, help='Execute fio scripts in fio_tests/')

    parser.add_argument('-a', '--all', dest='action', action='store_const',
                        const=all, help='Execute all fio tests.')

    parser.add_argument('-m', '--minimal', action='store_true',
                        help='Execute fio with --minimal and parse output to \
                        format: iotype;bs;njobs;iodepth;iops;slatmin; \
                        slatmax;slatavg;clatmin;clatmax;clatavg;latmin; \
                        latmax;latavg')

    parser.add_argument('-i', '--iterations', action='store',
                        help='Execute each test i times. This allows to exclude \
                        variances in the host where the tests are being \
                        executed. Execute fio with --minimal (good for \
                        scripting).')

    parser.add_argument('-o', '--output', action='store_true',
                        help='Store output in file with format: kernel_version- \
                        time(H-M-S)-fio.csv.')

    args = parser.parse_args()
    if args.action is None:
        parser.print_help()
        return

    if args.iterations is not None:
        n_iterations = int(args.iterations)


    if args.device is not None:
        lnvm_driver = args.device
    else:
        print "It is required to provide a device (-d)!\n"
        parser.print_help()
        return

    configure_paths(lnvm_driver, args)

    if args.output:
        f = initialize_file()
    else:
        f = None

    if not os.path.exists(lnvm_config):
        print ("lnvm_test: LightNVM not enabled!")
        return

    if os.path.exists(lnvm_file):
        result = subprocess.check_output(lnvm_remove_cmd, shell = True)

    result = subprocess.check_output(lnvm_config_cmd, shell = True)
    args.action(args, f)

    #Prevent removing lnvm_device too quickly in case of bad input
    time.sleep(2)

    result = subprocess.check_output(lnvm_remove_cmd, shell = True)

    if args.output:
        f.close

if __name__ == '__main__':
    main()
