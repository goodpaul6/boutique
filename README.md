# Boutique

A simple in-memory database. Stores collections of flat, pre-defined documents
that can be queried and written to easily.

## Current Goal Example

I want to be able to support the following use case.

Let's say I want to cache some user information, like their last login time and
last visited page.

```
# Define the schema
schema user {
      id                 string64
      last_login_time    uint64
      last_visited_page  string64
}

# Create a collection 'users' that contains 'user' schema documents, with id as their top-level key
coll users { user id }

# Create/update a user, note how we don't have to set the ID field, it is populated from the key
# parameter. These updates are atomic.
set users:user_id_here {
      last_login_time 1312309023
      last_visited_page '/dashboard'
}

# Retrieve just the last login time of the user
get users:user_id_here {
      last_login_time
}

# Delete this user
del users:user_id_here
```

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
- [x] Add support for timers to io (see timerfd on Linux)
- [x] Add tests for async_send and async_recv
- [ ] Allow defining schemas for key prefixes
- [ ] Add async_getaddrinfo to io
- [ ] Add async_connect to io
- [ ] Add async_listen to io
- [ ] Add tests for StreamBuf
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
