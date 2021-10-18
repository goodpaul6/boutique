# Boutique

A structured key-value store. You define types which will be stored at
predefined key prefixes and boutique stores and transmits them optimally.

## Commands

Currently `boutique` server receives the following commands over TCP:

```
GET key
=> Returns value stored at key

SET key value
=> Returns SUCCESS if successful
```


## TODO

+ Set up basic commands with an inline parser
- Binary protocol
- Key expiry
    - Involves implement (async) timers, see timerfd in linux
- Gracefully handle non-graceful disconnects from the client
    - Create a new exception type SocketError
