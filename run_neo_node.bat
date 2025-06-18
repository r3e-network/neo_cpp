@echo off
echo Starting Neo C++ Node...
echo.

simple-neo-node.exe > neo_node_output.txt 2>&1

echo.
echo Neo node execution completed. Check neo_node_output.txt for details.
pause 