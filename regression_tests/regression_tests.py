import subprocess
import os
import pandas as pd
import sys

import multiprocessing as mp
from multiprocessing import Pool


bin_path = "../build/"

# a function to run a command and
# parse the output.
def run_command(cmd):

    # using the Popen function to execute the
    # command and store the result in temp.
    # it returns a tuple that contains the
    # data and the error if any.
    print("cmd running:", cmd,)
    temp = subprocess.Popen(cmd, shell=True, stdout = subprocess.PIPE)

    # we use the communicate function
    # to fetch the output
    output = str(temp.communicate())

    # splitting the output so that
    # we can parse them line by line
    output = output.split("\n")

    output = output[0].split('\\')

    # a variable to store the output
    res = []

    # iterate through the output
    # line by line
    for line in output:
        res.append(line)

    return res

def generate_cs_mesh(res, filename):
    return

def generate_ico_mesh(res, filename, dual=False):
    return

def generate_rll_mesh(res, filename):
    return

def generate_overlap_mesh(inpfname1, inpfname2, method, outfname):
    command = []
    command.append(bin_path+"GenerateOverlapMesh")
    command.append("--a")
    command.append(inpfname1)
    command.append("--b")
    command.append(inpfname2)
    command.append("--method")
    command.append("exact")

    return command

# order: list with 2 entries that is strtictly above 0: FV (1:4), SE==4
# methods: list with 2 entries specifying one of fv, cgll, dgll
def generate_offline_map(inpfname1, inpfname2, inpoverlapmesh, outputmap, orders, methods, correct_areas, monotone):
    return

def generate_test_data(inpfname, testname, variablename):

    return

def apply_offline_map(mapfile, inputdatafile, variablename):
    return

def parse_help(res):
    opts = []
    for i in range(1, len(res) - 1):
      line = res[i]
      #print("line", i, line)
      a = line.find("--")
      if a != -1:
        opt = line.partition("--")[2].split()[0]
        opts.append(opt)
        # print("line is", opt)

    return opts

def create_results(res):
    opts = []
    for i in range(1, len(res) - 1):
      line = res[i]
      #print("line", i, line)
      a = line.find("....")
      if a != -1:
        opt = line.partition("....")[2].split()[2]
# ['Source', 'Mass:', '2.513274122871826e+01', 'Min', '1.0027367453e+00', 'Max', '2.9972632547e+00']
        opts.append(opt)
        print(line.partition("....")[2].split()[0], " ", opt)

    return opts


def build_mesh_args1(meshstr):
    if "cs" in meshstr:
        filename = "outCSMesh"
        command = "GenerateCSMesh"
        value = meshstr.partition("cs")[2]
        args=""
    elif "icod" in meshstr:
        filename = "outICOMesh"
        command = "GenerateICOMesh"
        value = meshstr.partition("icod")[2]
        args = " --dual"

    res = value
    filename+=res+".g"
    args= " --file "+ filename
    args+= " --res "+res
    return command, args, filename


# Worklow
## Parse ini file: compute unique CS, ICO, ICOD, RLL res
## Generate the mesh
## Generate test data if in source
## wait

# Do the apply regression workflow
## Generate overlap mesh
## Generate offline map
## Apply offline map
## Compare against baseline

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: \npython regression_tests.py <location of executables>\n\n")
        exit()

    bin_path = sys.argv[1] + "/"

    # These option are availble for checking and extending this regression testing commandline options
    res = run_command(bin_path+'GenerateCSMesh')
    gCSMesh_tokens = parse_help(res)
    res = run_command(bin_path+'GenerateICOMesh')
    gICOMesh_tokens = parse_help(res)
    res = run_command(bin_path+'GenerateOverlapMesh')
    gOverlapMesh_tokens = parse_help(res)
    res = run_command(bin_path+'GenerateOfflineMap')
    gOfflineMap_tokens = parse_help(res)
    res = run_command(bin_path+'GenerateTestData')
    gTestData_tokens = parse_help(res)
    res = run_command(bin_path+'ApplyOfflineMap')
    gApplyOfflineMap_tokens = parse_help(res)

    # Run a pipeline
    # read inputs
    command = []
    args=[]
    # space seperated table with keywords
    tm = pd.read_table('./test_matrix.ini', delim_whitespace=True, comment='#')

    # figure out order of commands to call
    # For each pipeline:
    # this loop gets the commands to call and the arguments, size of both commands and arguments is same

    for i in range(len(tm.values)):
# Command 1 & 2 for source/target mesh
        srcmesh_str =  tm.loc[i].at["srcmesh"]
        tgtmesh_str = tm.loc[i].at["tgtmesh"]
        cmd, arg, in_file = build_mesh_args1(srcmesh_str)
        command.append(cmd)
        args.append(arg)
        cmd, arg, out_file = build_mesh_args1(tgtmesh_str)
        command.append(cmd)
        args.append(arg)

# Command 3
        command.append("GenerateOverlapMesh")
        arg = " --a "+ in_file
        arg+= " --b " + out_file
        arg+= " --method exact"

        args.append(arg)

# Command 4
        command.append("GenerateOfflineMap")
        arg = " --ov_mesh overlap.g"
        order = str(tm.loc[i].at["order"])
        arg+= " --in_np "+ order+ " --out_np "+ order
        arg+= " --out_mesh " + out_file+ " --in_mesh "+ in_file
        ofmap_out="mapNE-"
        ofmap_out+=srcmesh_str.upper()+"-"+tgtmesh_str.upper()
        ofmap_out+="-O"+order+".nc"
        arg+= " --out_map " + ofmap_out

        args.append(arg)

# Command 5
        command.append("GenerateTestData")
        arg = " --mesh "+ in_file
        test = str(tm.loc[i].at["test"])
        arg+= " --test "+ test
        out_td="test"
        #srcmesh
        out_td+=srcmesh_str.upper()
        out_td+="-F"+test+"-O"+order+".nc"
        arg+= " --out " + out_td

        args.append(arg)

# Command 6
        command.append("ApplyOfflineMap")
        arg = " --in_data "+ out_td
        arg+= " --map "+ ofmap_out
        arg+= " --var Psi"
        out_test="OutTest"
        out_test+=srcmesh_str.upper()
        out_test+="-F"+test+"-O"+order+".nc"
        arg+= " --out_data " + out_test

        args.append(arg)

#TODO: unused matrix options


        tm.loc[i].at["correctareas"]
        tm.loc[i].at["monotone"]

#TODO: can be parallized
    print("args =",args)
    print("commands:", command)

    # loop thru all commands and run
    # check if the same command has already been run
    #path = "../build/"
    num_commands = len(command)
    res = []
    cmd_args_list = []
    j=0
    for i in range(num_commands):
        cmd=path+command[i]
        cmd_args = cmd+args[i]
       # Actual run happens here
        # Don't run cmd_args if already run
        if cmd_args not in cmd_args_list:
            res.append(run_command(cmd_args))
            j+=1
            print("-----DONE running command", j, "-----")

    # now that we've run once put on list to not run the same command again
        cmd_args_list.append(cmd_args)
    # store output and compareds = []
        if command[i] == "ApplyOfflineMap":
            results = create_results(res[j-1])
            diff = float(results[0]) - float(results[1])
            percentage_diff = ( diff * 100 )/float(results[0])
            print("Percentage diff between source and target is ", percentage_diff)

