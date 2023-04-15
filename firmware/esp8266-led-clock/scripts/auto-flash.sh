#!/bin/bash

PIOENV=lolin_d1_mini
export PATH=$PATH:~/.platformio/penv/bin 

function flash() {
    pushd ./..

    while :
    do 
        if platformio run --target upload --environment $PIOENV; then
            echo " firmware flashed!"
            break
        else
            echo "Failed, try again.."
        fi
        sleep 2
    done

    popd
}

echo "Looking for ttyUSB* device..."
while :
do
    if ls /dev/ttyUSB* 1> /dev/null 2>&1; then
        echo "Device ttyUSB* found, flashing firmware.."
        flash;
        echo -n "Flashed, waiting for diconnect..."

        while :
        do
            if ls /dev/ttyUSB* 1> /dev/null 2>&1; then
                echo -n "."
                sleep 1
            else
                echo -n "Diconnected, connect next device..."
                break
            fi
        done
    else 
        echo -n "."
        sleep 1
    fi
done