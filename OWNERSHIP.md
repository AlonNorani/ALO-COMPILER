# ALO Ownership System

## Overview

The ALO compiler implements a simplified ownership system to manage memory safety and prevent common errors:
- Double free
- Use-after-free  
- Data races
- Invalid mutations

## Core Concepts

### Stack Types (Copy Semantics)
These types are always copied and do not participate in the ownership system:
- `int`
- `float`
- `bool`

**Behavior**: Assignment creates a deep copy. Multiple variables can reference the same value independently.

```alo
let a = 5;
let b = a;  // Copies value 5
a = 10;     // Does not affect b
```

### Heap Types (Ownership Semantics)
These types are owned by a single variable:
- `string`
- `array`

**Behavior**: Only one variable can own a heap value at a time. Assignment transfers ownership (moves).

```alo
let s = "hello";
let s2 = s;  // Moves ownership to s2
// s is now invalid and cannot be used
```

## Ownership States

Each heap variable tracks its ownership state:

| State | Meaning | Valid Operations |
|-------|---------|------------------|
| `OWNED` | Variable owns the heap data | Read, write (if mutable), move, borrow |
| `BORROWED_IMM` | Immutably borrowed (read-only reference) | Read only |
| `BORROWED_MUT` | Mutably borrowed (exclusive reference) | Read and write |
| `MOVED` | Ownership transferred elsewhere | ERROR - cannot use |

## Borrowing Rules

A heap variable can be borrowed instead of moved:

### Immutable Borrow (`&T`)
- Multiple immutable borrows allowed simultaneously
- Prevents the original variable from being mutated or moved
- Read-only access to the borrowed data

### Mutable Borrow (`&mut T`)
- Only one mutable borrow can exist at a time
- Exclusive access: prevents other borrows and moves
- Allows both reading and writing the borrowed data

### Constraints

1. **No moves while borrowed**
   ```
   ERROR: Cannot move a variable while it is borrowed
   ```

2. **No simultaneous mutable borrows**
   ```
   ERROR: Cannot mutably borrow a variable twice
   ```

3. **No immutable borrow while mutably borrowed**
   ```
   ERROR: Cannot immutably borrow while mutably borrowed
   ```

4. **No mutation while immutably borrowed**
   ```
   ERROR: Cannot mutate variable that is immutably borrowed
   ```

## Operations Table

| Operation | Stack Type | Heap Type | Result |
|-----------|-----------|-----------|--------|
| `let b = a` | Copy | Move | Stack: both valid; Heap: a is moved |
| `copy(a)` | Copy | Deep copy | Both types: new independent copy |
| `&a` | N/A | Immutable borrow | Borrow reference, original valid but read-only |
| `&mut a` | N/A | Mutable borrow | Exclusive reference, original paused |
| Function arg `T` | Copy | Move | Heap: moved to function |
| Function arg `&T` | N/A | Immutable borrow | Read-only access in function |
| Function arg `&mut T` | N/A | Mutable borrow | Exclusive access in function |

## Implementation Details

### Symbol Table Extension

Each variable in the symbol table now tracks:
- `ownershipState`: Current ownership state (OWNED, BORROWED_IMM, BORROWED_MUT, MOVED)
- `activeBorrows`: List of active references to this variable
- `isHeapType()`: Whether this is a heap type

### Type System Integration

The parser's Parameter struct now includes:
- `borrowType`: "" (owned/move), "imm" (&T), or "mut" (&mut T)

Example function signature with borrows:
```alo
fn process(imm_ref: &string, mut_ref: &mut array) -> void { ... }
fn consume(owned_str: string) -> void { ... }
```

### Validation Flow

1. **Variable Declaration**: Initialized as OWNED
2. **Assignment**:
   - Stack types: Copied (no state change)
   - Heap types: Ownership transferred (source marked MOVED)
3. **Variable Use**: Check that state is not MOVED
4. **Borrowing**: Validate borrow rules, track active borrows
5. **Scope Exit**: Clean up borrows for scope-exiting variables

## Error Messages

The compiler provides precise error messages with line and column numbers:

```
Variable 'x' has been moved and cannot be used at line 5, column 10
Cannot mutably borrow 'x' while it is already borrowed at line 8, column 5
Cannot mutate 'x' while it is borrowed at line 12, column 3
```

## Future Enhancements

1. **Reference Types**: Full syntax support for `&T` and `&mut T` in declarations and function parameters
2. **Lifetime Annotations**: For complex reference scenarios
3. **Smart Pointers**: Box, Rc, RefCell equivalents
4. **Destructors**: Custom cleanup on scope exit
5. **Move Semantics Optimization**: Compiler hint for RVO (Return Value Optimization)

## Current Limitations

- Borrow syntax (`&` and `&mut`) not yet exposed in source code - implemented at semantic analysis level
- No explicit reference type declarations in source (planned for later)
- Borrows are implicit in function calls based on parameter type
- No lifetime bounds enforcement

## Examples

### Example 1: Stack Type - Always Valid

```alo
fn main() -> void {
    let a = 5;
    let b = a;
    let c = a;
    println!("{} {} {}", a, b, c);  // OK: 5 5 5
}
```

### Example 2: Heap Type - Move on Assignment

```alo
fn main() -> void {
    let s1 = "hello";
    let s2 = s1;        // Moves ownership to s2
    println!("{}", s1); // ERROR: s1 has been moved
}
```

### Example 3: Array Ownership

```alo
fn main() -> void {
    let arr1 = [1, 2, 3];
    let arr2 = arr1;    // Moves ownership
    // arr1[0] = 5;    // ERROR: arr1 has been moved
    println!("{}", arr2[0]);  // OK
}
```

### Example 4: Function Parameters (Future)

```alo
fn read_string(s: &string) -> void {
    // s is immutably borrowed - read-only
    println!("{}", s);
}

fn modify_array(arr: &mut array) -> void {
    // arr is mutably borrowed - can read and write
    arr[0] = 999;
}

fn consume_string(s: string) -> void {
    // s is moved into the function
    // Original variable cannot be used after call
}
```
