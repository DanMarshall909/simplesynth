#!/bin/bash
# Generate Mary Had a Little Lamb with sustained notes
# Just send Note Ons and let OfflineRenderer sustain them

note_on() {
    local note=$1
    printf "\x90\x$(printf '%02x' $note)\x64"  # Note On, velocity 100
}

# First phrase: E D C D E E E (each note ~1.1 seconds with sustain)
note_on 64
note_on 62
note_on 60
note_on 62
note_on 64
note_on 64
note_on 64

# Rest (implicit via duration limit)

# Second phrase: E G A
note_on 64
note_on 67
note_on 69

# Third phrase: A A A G E D C
note_on 69
note_on 69
note_on 69
note_on 67
note_on 64
note_on 62
note_on 60
