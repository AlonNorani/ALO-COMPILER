#!/bin/bash
mkdir -p should_pass

# Original tests wrapped in main
cat <<'EOF' > should_pass/test1.alo
fn main() -> void {
    let x = 5;
}
EOF

cat <<'EOF' > should_pass/test2.alo
fn main() -> void {
    let x = 5;
    let y = 10;
}
EOF

cat <<'EOF' > should_pass/test3.alo
fn main() -> void {
    let mut x = 5;
    x = 6;
}
EOF

cat <<'EOF' > should_pass/test4.alo
fn main() -> void {
    let x = 5;
    let y = x + 3;
}
EOF

cat <<'EOF' > should_pass/test5.alo
fn main() -> void {
    let x = 5;
    if (x > 3) {
        let y = 1;
    }
}
EOF

cat <<'EOF' > should_pass/test6.alo
fn main() -> void {
    let x = 5;
    if (1) {
        let y = 2;
    }
}
EOF

cat <<'EOF' > should_pass/test7.alo
fn main() -> void {
    let x = 5;
    if (1 && x) {
        let y = 3;
    }
}
EOF

cat <<'EOF' > should_pass/test8.alo
fn main() -> void {
    let mut x = 1;
    while (x < 5) {
        x = x + 1;
    }
}
EOF

cat <<'EOF' > should_pass/test9.alo
fn main() -> void {
    let x = 5;
    {
        let x = 10;
    }
}
EOF

cat <<'EOF' > should_pass/test10.alo
fn main() -> void {
    let x = true;
    let y = false;
    let z = x && y;
}
EOF

cat <<'EOF' > should_pass/test11.alo
fn main() -> void {
    let x = 5;
    let y = 10;
    let z = x < y;
}
EOF

cat <<'EOF' > should_pass/test12.alo
fn main() -> void {
    let x = 5;
    let y = 3;
    let z = x == y;
}
EOF

cat <<'EOF' > should_pass/test13.alo
fn main() -> void {
    let x = 5;
    let y = -x;
}
EOF

cat <<'EOF' > should_pass/test14.alo
fn main() -> void {
    let x = true;
    let y = !x;
}
EOF

cat <<'EOF' > should_pass/test15.alo
fn main() -> void {
    let x = 5;
    {
        let mut y = x + 2;
        y = y + 3;
    }
}
EOF

# New passing tests
cat <<'EOF' > should_pass/test16.alo
fn main() -> void {
    let x = 5;
    let y = 10;
    let z = x + y;
}
EOF

cat <<'EOF' > should_pass/test17.alo
fn main() -> void {
    let x = true;
    if (!x) {
        let y = 1;
    }
}
EOF

cat <<'EOF' > should_pass/test18.alo
fn main() -> void {
    let mut x = 0;
    while (x < 10) {
        x = x + 1;
    }
}
EOF

cat <<'EOF' > should_pass/test19.alo
fn main() -> void {
    let x = 5;
    let y = 10;
    let z = x * y;
}
EOF

cat <<'EOF' > should_pass/test20.alo
fn main() -> void {
    let x = 5;
    let y = 10;
    let z = x / y;
}
EOF

echo "Created 20 passing tests in should_pass/"