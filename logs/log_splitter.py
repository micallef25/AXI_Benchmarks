import sys
import os
from pathlib import Path

# -----------------------
# Settings              #
# -----------------------
filter_string = 'Format:'
ignore = '=~=~=~='
# -----------------------


def split(log_filepath): 
    """
    Split a file by the given filter_string, given a valid filepath.
    """
    
    # Get all the path info.
    log_filepath = Path(log_filepath)
    print(log_filepath)
    folder = log_filepath.parent
    base = log_filepath.stem
    ext = log_filepath.suffix
    
    # Initialize & open the file.
    file_contents = ''
    increment = 1
    with open(log_filepath, 'r') as log:
        
        # Iterate through the lines.
        for line in log:

            # Start of a new log; save off the old one 
            # (if it exists) and start over.
            if line.startswith(filter_string) and file_contents:
            
                new_filepath = os.path.join(folder, base + '_' + str(increment) + ext)
            
                with open(new_filepath, 'w') as split_log:
                    split_log.write(file_contents)
                    increment += 1
                    file_contents = line
                
            # Don't add this line to the split log.
            elif line.startswith(ignore):
                pass
            
            # Add it as per usual.
            else: 
                file_contents += line
                
    # Save off the final split log.
    new_filepath = os.path.join(folder, base + '_' + str(increment) + ext)
    with open(new_filepath, 'w') as split_log:
        split_log.write(file_contents)

    print('Split {log_filepath} into {increment} file(s).')


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
    split(log_filepath)    
    