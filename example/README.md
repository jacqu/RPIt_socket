## Introduction

The goal of this example is to illustrate how to interface a sensor
with rpit. The measurements from a sensor are modeled by the program
`sensor_stream` which streams some fake measurements data through
UDP. These measurements are read by the `rpit_socket_server` program,
where they are packaged into the `RPIt_socket_mes_struct`, and sent
through UDP to the *target* (usually a Raspberry Pi).

![Architecture](architecture.svg)

To make a test : 

1. Compile by typing `make` into a terminal.
2. Run `./sensor_stream` on a **first** terminal. 
3. Run `./rpit_socket_server` on a **second** terminal 
4. Compile the simulink and deploy to the target.

## sensor_stream 

The IP address, port, and message frequency can be changed in the
defines at the beginning of the `sensor_stream.c` file.

The sensor sends a string with the following format:
`PointID,x,y,z,timestamp`, where `PointID` is a string, `x`, `y`, and
`z` are doubles, and `timestamp` is an unsigned integer.

## rpit_socket_server

This programs launches a thread `rpit_socket_server_update` that
continuously reads the measurements from the sensor and updates the
`mes` structure, which is protected by the mutex `mes_mutex`.

In the `main` function there is an infinite loop that receives and
validates the measurements from the target. These measurements are put
into the `con` structure. Then, the `mes` structure is sent to the
target.

Finally, in the `rpit_socket_server_udpate` thread, if the
measurements coming from the target are not updated after some time
(the connection with the target is probably lost), the control signals
in the `con` structure are forced to 0.0.

The `mes_mutex` is used to protect both the `mes` and `con`
structures.

## sensor_utils

The file `sensor_utils.c` contains the functions to receive and parse
the sensor messages to a format compatible with the
`RPIt_socket_mes_struct` structure. For now only the `x`, `y`, and `z`
are used. Parsing the rest of the message is left as an exercice :).

## Useful Ressources 

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

- [The ANSI C Programming Language](https://archive.org/details/the-ansi-c-programming-language-by-brian-w.-kernighan-dennis-m.-ritchie.org/page/22/mode/2up)
