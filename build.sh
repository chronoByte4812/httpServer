echo "Started building binary...\n";

# sudo apt update
# sudo apt install -y libssl-dev

g++ -std=c++20 ./main.cpp -o server # -lssl -lcrypto

if [ $? -eq 0 ]; then echo "Nice the binary was successfully built!";
else echo "\nBinary failed to build!";
fi

if [[ "$1" == "-r" ]]; then
    echo "-r flag! the binary has started";
    ./server
fi