#!/bin/bash

qemu-system-aarch64 -machine raspi3 -nographic -serial null -serial mon:stdio -m size=1G -kernel $basedir/kernel.img
