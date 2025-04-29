#!/bin/bash
#validate arguemnt number.
if [ $# -ne 2 ]; then 
    echo "Usage: $0 <source_pgn_file> <destination_directory>"
    exit 1
fi
#validate source file existance
if [ ! -e "$1" ]; then
    echo "Error: file '$1' does not exist."
    exit 1
fi
#validate desitination (create on if doesnt exist)
if [ ! -d "$2" ]; then
    if ! mkdir -p "$2"; then 
        echo "Error: failed to create directory '$2'" 
        exit 1
    fi
    echo "Created directory $2"
fi 

input_file_name=$(basename "$1") #get filename without path 
input_file_name="${input_file_name%.*}" #remove extention
output_dir=$2
game_count=0 

while IFS= read -r line || [ -n "$line" ]; do
    if [[ "$line" == [Event\ * ]]; then
        game_count=$((game_count+1))
        output_file="${output_dir}/${input_file_name}_${game_count}.pgn"
        echo "Saved game to ${output_file}"
    fi
    echo "$line" >> "${output_file}"
done < "$1" #loop over lines of the input_file
echo "All games have been split and saved to '${output_dir}'"
exit 0
