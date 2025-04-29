#!/bin/bash
#check for correct argument count.
if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <PGN_FILE>"
    exit 1
fi
#check if source file exists and is a regular file.
if [ ! -f "$1"  ]; then
    echo "File does not exist: $1"
    exit 1
fi
#init board, and store a copy of the first board.
board_zero=(
    "r n b q k b n r"
    "p p p p p p p p"
    ". . . . . . . ."
    ". . . . . . . ."
    ". . . . . . . ."
    ". . . . . . . ."
    "P P P P P P P P"
    "R N B Q K B N R"
)
board=("${board_zero[@]}")
MOVES=() #contains all parsed moves in UCI
BOARDS=() #contains all the possible board states.
MOVE_COUNT=0 #number of moves in the game.

print_board() {

    #print column labels
    echo "  a b c d e f g h"

    #print the board with row numbers
    for i in {0..7}; do
        printf "%d %s %d\n" "$((8 - i))" "${board[i]}" "$((8 - i))"
    done

    #print column labels again
    echo "  a b c d e f g h"
}

move_piece() {
    local og_x=$(( 8 - $1 ))
    local og_y=$2
    local new_x=$(( 8 - $3 ))
    local new_y=$4
    local special=$5

    #get absolute position in sting.
    local colindex=$(( (og_y - 1) * 2 ))
    local newColindex=$(( (new_y - 1) * 2 ))
    local piece=${board[$og_x]:colindex:1}

    #calculate the distance traveld vertically and horizontally, usefull for detecting special cases.
    local vdist=$((og_x - new_x))
    vdist=${vdist#-}
    local hdist=$((og_y - new_y))
    hdist=${hdist#-}
    #CASTLING, when a king moves more than one square horizontally.
    #We add the special flag to avoid an infinite loop.
    if [[ "${piece,,}" == "k" ]] && [[ $hdist -gt 1 ]] && [[ -z "$special" ]]; then
        #Castling to the left.
        if [[ $og_y -gt $new_y ]]; then
            if [[ "$piece" == "k" ]]; then play_move "e8c8" "1"; play_move "a8d8" "1";
            else play_move "e1c1" "1"; play_move "a1d1" "1"; fi
        fi

        #Castling to the right.
        if [[ $og_y -lt $new_y ]]; then
            if [[ "$piece" == "k" ]]; then play_move "e8g8" "1"; play_move "h8f8" "1";
            else play_move "e1g1" "1"; play_move "h1f1" "1"; fi
        fi
        return 0
    fi
    #PROMOTION, if the special flag is non-empty and we move a pawn, 
    #that means the original movecode had 5 characters, i.e promotion.
    if [[ -n "$special" ]]; then
        if [[ "$piece" == "p" ]]; then piece=${special,,}; fi
        if [[ "$piece" == "P" ]]; then piece=${special^^}; fi
    fi
    #ENPASSANTE, when a pawn moves diagonally to an empty square,
    #which can only happen during enpassante.
    if [[ "${piece,,}" == "p" ]] && [[ vdist -eq 1 ]] && [[ hdist -eq 1 ]]; then
        if [[ "${board[$new_x]:newColindex:1}" == "." ]]; then
            #empty the bellow or above
            board[$og_x]=${board[$og_x]:0:newColindex}"."${board[$og_x]:$((newColindex + 1))}
        fi
    fi
   
    #Update board at new position.
    board[$new_x]=${board[$new_x]:0:$newColindex}$piece${board[$new_x]:$((newColindex + 1))}
    #Remove character at old position.
    board[$og_x]=${board[$og_x]:0:$colindex}"."${board[$og_x]:$((colindex + 1))}
}

letter_to_number() {
    case $1 in 
        a) echo 1;;
        b) echo 2;;
        c) echo 3;;
        d) echo 4;;
        e) echo 5;;
        f) echo 6;;
        g) echo 7;;
        h) echo 8;;
        *) echo "Error: $1 is not a valid column"; exit 1;;
    esac
}

play_move() {
    local movecode=$1
    local special=$2
    if [[ ${#movecode} -lt 4 ]] || [[ ${#movecode} -gt 5 ]]; then
        echo "Error: not a vaid move, must have 4 or 5 characters"
        exit 1
    fi
    #pasrse the movecode into numbers.
    local current_pos=${movecode:0:2}
    local move_to=${movecode:2}
    local letter=${current_pos:0:1}
    local current_row=${current_pos:1:1}
    local current_col=$(letter_to_number "$letter")
    local new_row=${move_to:1:1}
    letter=${move_to:0:1}
    local new_col=$(letter_to_number "$letter")
    if [[ $current_row -ge 9 || $current_row -le 0 ]]; then
        echo "Error: $current_row is not a valid row"
        exit 1
    fi
    if [[ $new_row -ge 9 || $new_row -le 0 ]]; then
        echo "Error: $new_row is not a valid row"
        exit 1
    fi

    #call move_piece with the parsed values.
    if [[ ${#movecode} -eq 5 ]]; then special=${movecode:4:1}; fi
    move_piece "$current_row" "$current_col" "$new_row" "$new_col" "$special"
}

#Prints out PGN metadata and saves the parsed moves into the MOVES array and their count to MOVE_COUNT variable.
process_pgn() {
    local source_pgn_file=$1
    local moves_found=0
    local moves_text=""
    while IFS= read -r line; do
        if [[ -z $line ]]; then
            moves_found=1
            continue
        elif [[ $moves_found -eq 0 ]]; then
            echo "$line"
        else
            moves_text+="$line "
        fi
    done < "$source_pgn_file"
    local tmp_moves=$(python3 parse_moves.py "$moves_text")
    MOVES=($tmp_moves)
    MOVE_COUNT=${#MOVES[@]}
}

#append the current board stored inside the board array to the BOARDS array.
save_board() {
    BOARDS+=("$(printf "%s\n" "${board[@]}")") 
}

#stores the value of a board from the BOARDS array at a certain index inside the board array.
restore_board_from_history() {
    local index=$1
    IFS=$'\n' read -r -d '' -a board <<< "${BOARDS[$index]}"
}

#fill the BOARDS array with all the possible board states. BOARDS[i] will be the board after playing the i'th move.
fill_board_history() {
    BOARDS+=("$(printf "%s\n" "${board_zero[@]}")") 
    for ((i = 0; i < MOVE_COUNT; i++)); do
        play_move "${MOVES[$i]}";
        save_board
    done;
}

#Main loop of the program. asks the user for input, and then queries the BOARDS array depending on that input.
play_turn() {
    local move_num=$1
    local print_board_flag=$2
    if [ -z "$print_board_flag" ]; then
        echo "Move ${move_num}/${MOVE_COUNT}"
        print_board
    fi
    echo -n "Press 'd' to move forward, 'a' to move back, 'w' to go to the start, 's' to go to the end, 'q' to quit:" 
    read -r opt 
    echo ""
    case $opt in 
        d)
            if [ "$move_num" -eq "$MOVE_COUNT"  ]; then
                echo "No more moves available."
                play_turn "$MOVE_COUNT" "1"
            else
                restore_board_from_history "$((move_num + 1))" 
                play_turn "$((move_num + 1))";
            fi;;
        s)
            if [ "$move_num" -eq "$MOVE_COUNT"  ]; then
                play_turn "$MOVE_COUNT"
            else
                restore_board_from_history "$MOVE_COUNT"
                play_turn "$MOVE_COUNT";
            fi;;
        w)
            board=("${board_zero[@]}")
            play_turn "0";;
        a)
            if [ "$move_num" -eq 0 ]; then
                play_turn "0"
            else
                restore_board_from_history "$((move_num - 1))"
                play_turn "$((move_num - 1))";
            fi;;
        q)
            echo "Exiting."
            echo "End of game.";;
        *)
            echo "Invalid key pressed: ${opt}";
            play_turn "$move_num" "1";;
    esac
}


echo "Metadata from PGN file:"
process_pgn "$1"
echo ""
fill_board_history #initialy fill the BOARDS array.
board=("${board_zero[@]}")
play_turn "0"
