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

- [x] Set up basic commands with an inline parser
- [x] Binary protocol
- [ ] Add support for timers to io (see timerfd on Linux)
- [ ] Add async_connect to io
- [ ] Add async_listen to io
- [ ] Allow defining schemas for key prefixes
- [ ] Key expiry using async timers
- [ ] Separate server from database so that the latter can be embedded in-process
- [ ] Gracefully handle non-graceful disconnects from the client
- [ ] Create a new exception type SocketError
- [ ] Create a "smart" client which allows for higher performance through eventual
      consistency by having updates to recently accessed keys be streamed back
      from the server. This should be transparent to users, they just allow eventual
      consistency.
- [ ] Smart client which streams updates back to the server in a separate thread
      so there's no write overhead
- [ ] Switch to using epoll instead of select on Linux
- [ ] Use std:: prefix everywhere for cstdint types
