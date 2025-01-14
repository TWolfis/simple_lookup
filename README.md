# simple_lookup
simple_lookup is a C program to experiment with DNS lookups

## Build
To build the project, you can use the following command:

```bash
clang -o simple_lookup simple_lookup.c -Wall -Wextra -Werror -lresolv
```
This will produce the executable simple_lookup.

Usage
Usage: ./simple_lookup <hostname>
simple_lookup takes a hostname as its only argument.

## Example
```bash
./simple_lookup google.com
```

## License
BSD-3-Clause


Bronnen en gerelateerde content

