echo "Started building server binary...\n";

g++ -std=c++20 ./main.cpp -o server # -lssl -lcrypto

if [ $? -eq 0 ]; then echo "Nice the binary was successfully built!";
else echo "\nBinary failed to build binary";
fi