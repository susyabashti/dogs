#pragma once

#include <libpq-fe.h>
#include <stddef.h>

// Initialize the connection pool with 'size' connections.
// Returns 0 on success, non-zero on failure.
int db_pool_init(size_t size);

// Register a prepared statement BEFORE pool init
int db_register_prepared(const char *name, const char *sql);

// Get a connection from the pool (blocking if none available).
PGconn *db_pool_acquire(void);

// Return a connection to the pool.
void db_pool_release(PGconn *conn);

// Close all connections and free the pool.
void db_pool_destroy(void);
