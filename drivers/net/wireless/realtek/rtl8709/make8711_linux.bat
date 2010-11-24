#!/bin/sh

echo "[cp Makefile_linux Makefile]"
cp Makefile_linux Makefile

echo "[make]"
make

echo "[rm Makefile]"
rm Makefile
