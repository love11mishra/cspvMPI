import os
import shutil
import subprocess
import argparse


def main(program):
    # Set crest and pat dirs
    crest_dir = "/home/lavleshm/Academic/mpi_verify/crest-0.1.1/"
    pat_dir = "/home/lavleshm/Downloads/Process\ Analysis\ Toolkit\ 3.5.1/"
    # run_command indicates the concolic execution command using crest
    run_command = crest_dir + "bin/run_crest " + str(program.n) + " ./"
    run_command += str(program.executable) + " " + str(program.iters) + " -dfs"
    # Executing the program
    os.system(run_command)
    # collecting all the newly created MpiTrace files
    traces = subprocess.check_output("ls MpiTrace_*.txt", shell=True)
    traces = list(traces.decode("utf-8").split("\n")[:-1])
    # starting verification for each of the trace files
    for i in range(1, len(traces) + 1):
        # reomve pre-existing output_dir and create a new one
        output_dir = os.getcwd() + "/vfnOut" + str(i)
        shutil.rmtree(output_dir, ignore_errors=True)
        os.makedirs(output_dir)
        os.system("mv " + traces[i - 1] + " " + output_dir + "/")
        trace_file = output_dir + "/" + traces[i - 1]
        # csp output file to be created by translator_command
        csp_file = output_dir + "/output" + str(i) + ".csp"
        translator_command = "python3 " + crest_dir + "/../src/Translator1.py "\
            + trace_file + " " + csp_file
        # creating csp program form MpiTrace
        os.system(translator_command)
        # print(translator_command)
        # print("translation done!")
        PAT = pat_dir + "PAT3.Console.exe"
        # cspVerification result is stored csp_output_file
        csp_output_file = output_dir + "/csp_res" + str(i) + ".txt"
        pat_command = "mono " + PAT + " -v " + csp_file + " " + csp_output_file
        # print(pat_command)
        os.system(pat_command)
        # Verification result analysis
        lines = []
        with open(csp_output_file, 'rt') as f:
            lines = f.readlines()
        for line in lines:
            if (line[:5] == '<init'):
                pass


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='''
    This script is used to execute an instrumented MPI program and perform \
    trace verification.
    ''')
    parser.add_argument('n',
                        help='Number of processes(same as -n argument \
                        of mpirun/mpiexec command)')
    parser.add_argument('executable',
                        help=' MPI_Program(executable) followed by \
                        its arguments')
    parser.add_argument('--iters',
                        help='Number of iterations(default = 1) of concolic \
                        execution',
                        default=1)
    args = parser.parse_args()
    main(args)
