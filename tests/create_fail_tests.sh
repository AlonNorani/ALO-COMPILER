#!/bin/bash
mkdir -p should_fail

# Original tests wrapped in main
cat <<'EOF' > should_fail/test1.alo
fn main() -> void {
    x = 5;
}
EOF

cat <<'EOF' > should_fail/test2.alo
fn main() -> void {
    let x = 5;
    x = 10;
}
EOF

cat <<'EOF' > should_fail/test3.alo
fn main() -> void {
    let x = 5;
    let x = 6;
}
EOF

cat <<'EOF' > should_fail/test4.alo
fn main() -> void {
    let x = true;
    let y = 5;
    let z = x + y;
}
EOF

cat <<'EOF' > should_fail/test5.alo
fn main() -> void {
    let x = 5;
    {
        let y = 10;
    }
    y = 3;
}
EOF

cat <<'EOF' > should_fail/test6.alo
fn main() -> void {
    let x = true;
    let y = -x;
}
EOF

cat <<'EOF' > should_fail/test7.alo
fn main() -> void {
    let x = 5;
    let y = !x + 2;
}
EOF

cat <<'EOF' > should_fail/test8.alo
fn main() -> void {
    let x = true;
    let y = false;
    let z = x < y;
}
EOF

cat <<'EOF' > should_fail/test9.alo
fn main() -> void {
    let x = 5;
    let y = 10;
    let z = x && y + 3;
}
EOF

cat <<'EOF' > should_fail/test10.alo
fn main() -> void {
    let x = 5;
    let y = true;
    x = y;
}
EOF

cat <<'EOF' > should_fail/test11.alo
fn main() -> void {
    let x = true;
    if (x + 1) {
    }
}
EOF

cat <<'EOF' > should_fail/test12.alo
fn main() -> void {
    let x = 5;
    while (true + 1) {
    }
}
EOF

cat <<'EOF' > should_fail/test13.alo
fn main() -> void {
    let x = 5;
    {
        let x = true;
    }
    x = true;
}
EOF

cat <<'EOF' > should_fail/test14.alo
fn main() -> void {
    let x = true;
    let y = true;
    let z = x * y;
}
EOF

cat <<'EOF' > should_fail/test15.alo
fn main() -> void {
    let x = 5;
    let y = x + unknown;
}
EOF

# New failing tests
cat <<'EOF' > should_fail/test16.alo
fn main() -> void {
    let x = 5;
    let y = true;
    let z = x && y;
}
EOF

cat <<'EOF' > should_fail/test17.alo
fn main() -> void {
    let x = 5;
    let y = 0;
    let z = x / y;
}
EOF

cat <<'EOF' > should_fail/test18.alo
fn main() -> void {
    let x = true;
    x = false + 1;
}
EOF

cat <<'EOF' > should_fail/test19.alo
fn main() -> void {
    let x = 5;
    if (x) {
        let y = true;
    } else {
        let y = 1;
    }
    y = 3;
}
EOF

cat <<'EOF' > should_fail/test20.alo
fn main() -> void {
    let x = 5;
    let y = "hello";
    let z = x + y;
}
EOF

echo "Created 20 failing tests in should_fail/"