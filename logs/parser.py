import tkinter
import csv
from matplotlib import pyplot as plt
import sys
import os
from pathlib import Path


ignore = "Format:" 
ignore2 = "CACHE"
ignore3 = "DDR"
ignore4 = "OCM"

# Python program to get average of a list 
def Average(lst): 
    return sum(lst) / len(lst) 

def make_plot(log_filepath):

    reads = []
    read_trans = []
    writes = []
    write_trans = []
    port =""
    mem_type = ""

    with open(log_filepath, 'r') as csvfile:
        plots= csv.reader(csvfile, delimiter=',')
        

        for row in plots:

            if (row[0].startswith(ignore) or row[0].startswith(ignore2) or row[0].startswith(ignore3) or row[0].startswith(ignore4)):
                pass

            else:
                port = row[0]
                mem_type = row[1]
                read_tran = int(row[2],10)
                read = int(row[3],10)
                write_tran = int(row[4],10)
                write = int(row[5],10)
                #print("Avg Read Transaction {} {} {}".format(port,mem_type,float(write_tran)/float(write) ))
                #print("Avg Write Transaction {} {} {}".format(port,mem_type,float(read_tran)/float(read) ))
            # 100 000 000
            #print("{} Bps ".format( (float(read)/float(latency) ) * 100000000 )) 
                reads.append(read)
                read_trans.append(read_tran)
                writes.append(write)
                write_trans.append(write_tran)



        print("Avg Read Transaction {} {} {}".format(port,mem_type,float(Average(read_trans))/float(Average(reads)) ))
        print("Avg Write Transaction {} {} {}".format(port,mem_type,float(Average(write_trans))/float(Average(writes)) ))

        plt.plot(reads,read_trans, marker='o')

        plt.title('Results from AXI Performance Monitor')

        plt.xlabel('Bytes_Read')
        plt.ylabel('Total Latency')

        # plt.show()

if __name__ == '__main__':
    """
    Run 'python log_splitter.py filename'
     or 'python log_splitter.py path/to/file'
     
    Will produce a series of split logs named {basename}_1, {basename}_2, etc.
    All split logs will appear in the same folder as the original log.
    """
    
    if len(sys.argv) != 2:
        print('Don\'t do that, Eric. Just give me a file to split. :(')
        exit()
        
    # Get the filename from the system args.
    log_filepath = sys.argv[1]
    make_plot(log_filepath)   