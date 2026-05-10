// Uses Socket Programming
// Uses HTTP library

// Future Work - make a CLI using which files can be transferred 


To Compile any change in server

- cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- cmake --build build -j
- ./build/server  


Use netcat to send a request to the server
- nc 127.0.0.1 <PORT>