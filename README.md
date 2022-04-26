# Boutique

A simple in-memory database. Stores collections of flat, pre-defined documents
that can be queried and written to easily.

## CLI Example

_Note that the current CLI is intentionally barebones, and the syntax is all subject to change._

Let's say I want to cache some user information, like their last login time.

First, we register the schema

```
> schema
name > user
field name or end > id
field type > string
string capacity > 64
field name or end > last_login_time
field type > uint64
field name or end > end
key field name > id
Success.
```

This registers a schema with the database called `user` which contains an `id`, a `string` at most 64 bytes long, and
`last_login_time`, a `uint64` which stores milliseconds since unix epoch. I also specify that I want the `id` to be
the key for the

Next we create a collection which stores this schema

```
> collection
name > users
schema name > user
Success.
```

This creates a collection called `users` which stores documents with the `user` schema.

Next, we retrieve a copy of the schema from the database to facilitate insert and retrieve operations

```
> colschema
name > users
key id string64
last_login_time uint64
```

Running the `colschema` commands retrieves the schema for a collection, pretty printing it in the process.
Note that the `id` is the key.

Let's put a document in the database

```
> put
collection name > users
value for string id > user_1
value for uint64 last_login_time > 1650929781214
Success.
```

We just inserted/updated a `user` document with id `user_1` into the `users` collection. The CLI helpfully outlines
the fields we're providing values for.

Now we can retrieve this document

```
> get
collection name > users
key > user_1
id = "user_1"
last_login_time = 1650929781214
```

and that's basically a database, right?

## TODO

- [x] Set up basic commands with an inline parser
- [x] Binary protocol
- [x] Add support for timers to io (see timerfd on Linux)
- [x] Add tests for async_send and async_recv
- [x] Add storage object for storing documents
- [x] Add collections which can store documents and index them by key
- [x] Create database object for housing schemas and collections
- [x] Create open-addressed hash table as storage for collection
- [x] Profile and optimize collection data structure
- [x] Update protocol
- [x] Update server
- [x] Update CLI
- [x] Structured read of key type for `get` command in CLI
- [ ] Figure out a better way to handle 'find' with strings; we currently use ConstBuffer len instead
      of examining the buffer and embedded string length, which is technically inconsistent with how
      we treat all other values
- [ ] Add deletion command
- [ ] Add support for nested schemas
- [ ] Add support for arrays in schemas
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
