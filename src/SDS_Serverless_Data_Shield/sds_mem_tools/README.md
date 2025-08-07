# sdsmemtools

`sdsmemtools` is a Python extension written in C that provides tools for low-level memory inspection of Python objects. It allows you to get the memory address of an object and view the raw bytes of the object's in-memory representation.

## Building the Extension

This project uses CMake to build the C extension.

### Prerequisites

*   CMake 3.10 or higher
*   A C compiler (like GCC or Clang)
*   Python 3 development headers

### Build Steps

1.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

2.  **Run CMake to configure the project:**
    ```bash
    cmake ..
    ```

3.  **Build the shared library:**
    ```bash
    make
    ```

This will create a `sdsmemtools.so` file in the `build` directory. You can then import this file directly in Python.

## Usage

To use the tools, import the `sdsmemtools` module and use the `MemView` class.

```python
import sdsmemtools

# Create a MemView object
try:
    mem_view = sdsmemtools.MemView(value="Hello, SDS!")
    
    # Get the value back
    value = mem_view.value()
    print(f"Initial value: {value}")
    
    # Get the size in bytes
    size = mem_view.bsize()
    print(f"Byte size: {size}")
    
    # Clear the memory
    mem_view.clear()
    print(f"Value after clearing: {mem_view.value()}")

except TypeError as e:
    print(f"Error: {e}")

```

## API Reference

The `sdsmemtools` module exposes the `MemView` class.

### `sdsmemtools.MemView(value: str)`

Creates a new `MemView` object.

*   **Parameters:**
    *   `value` (str): The initial string value to store.

### `MemView.clear()`

Securely clears the memory content of the `MemView` object by overwriting it with zeros.

### `MemView.value() -> str`

Retrieves the value of the `MemView` object as a Python string.

### `MemView.bsize() -> int`

Returns the size of the data in bytes.

### `MemView.xor(other: MemView) -> MemView`

Performs a byte-wise XOR operation with another `MemView` object of the same size. Returns a new `MemView` object with the result.

### `MemView.lshift(shift: int) -> MemView`

Performs a bitwise left circular shift (rotation) on the data. Returns a new `MemView` object with the result.

### `MemView.concat(other: MemView) -> MemView`

Concatenates the `MemView` object with another one. Returns a new `MemView` object containing the concatenated data.

### `MemView.slicing(origin: int, offset: int) -> MemView`

Extracts a slice of bits from the data.
*   `origin`: The starting bit index.
*   `offset`: The number of bits to slice.
Returns a new `MemView` object with the sliced bits.

### `MemView.badd(other: MemView) -> MemView`

Performs byte-wise addition with another `MemView` object of the same size, handling carry-over. Returns a new `MemView` object with the result.

## Examples

### Creating and Inspecting a `MemView`

```python
import sdsmemtools

# Create a MemView
mv1 = sdsmemtools.MemView(value="test")
print(f"mv1 value: {mv1.value()}")
print(f"mv1 size: {mv1.bsize()}")
```

### Bitwise Operations

```python
import sdsmemtools

mv1 = sdsmemtools.MemView(value="Hello")
mv2 = sdsmemtools.MemView(value="TEST")

# XOR
xor_result = mv1.xor(mv2)

# Left Shift
lshift_result = mv1.lshift(shift=8) #lshift_result.value() == "ESTT"
```

### Concatenation and Slicing

```python
import sdsmemtools

mv1 = sdsmemtools.MemView(value="part1")
mv2 = sdsmemtools.MemView(value="part2")

# Concatenate
concat_result = mv1.concat(mv2)
print(f"Concatenated: {concat_result.value()}")
print(f"Concatenated size: {concat_result.bsize()}")

# Slice the first 3 bytes (24 bits)
slice_result = concat_result.slicing(origin=0, offset=24)
print(f"Sliced value: {slice_result.value()}")
```
