# Dogs API Service

## Build types

| Build type | Flags (GCC/Clang) | Purpose |
|---|---|---|
| `Debug` (default) | standard debug flags | local development |
| `Release` | `-O3` | production |
| `Sanitize` | `-fsanitize=address,undefined -fno-omit-frame-pointer` | catch memory errors (ASan) and undefined behavior (UBSan) |
| `SanitizeTSan` | `-fsanitize=thread -fno-omit-frame-pointer` | catch data races (TSan) |

Sanitizer flags are applied directory-wide, so the vendored civetweb/cJSON
static libraries are compiled and linked with them too (they keep their `-w`
warning suppression). ASan and TSan are mutually exclusive, which is why they
are two separate build types. On MSVC both sanitizer build types are no-ops.

### ASan + UBSan build

```sh
cmake -B build-asan -DCMAKE_BUILD_TYPE=Sanitize
cmake --build build-asan --parallel
```

Run it (e.g. against the seeded dev postgres):

```sh
./build-asan/dogs_api
# in another shell:
curl -f http://127.0.0.1:8080/api/health
curl -f "http://127.0.0.1:8080/api/dogs?limit=3"
```

A clean run prints no `ERROR: AddressSanitizer` / `runtime error:` output and
exits 0.

### TSan build

```sh
cmake -B build-tsan -DCMAKE_BUILD_TYPE=SanitizeTSan
cmake --build build-tsan --parallel
```

Run it and exercise the endpoints:

```sh
./build-tsan/dogs_api
# in another shell:
curl -f http://127.0.0.1:8080/api/health
curl -f "http://127.0.0.1:8080/api/dogs?limit=3"
```

A clean run prints no `WARNING: ThreadSanitizer` output and exits 0.
