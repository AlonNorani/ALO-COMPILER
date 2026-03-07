#!/bin/bash
mkdir -p should_fail

cat <<'EOF' > should_fail/test1.alo
x = 5;
EOF

cat <<'EOF' > should_fail/test2.alo
let x = 5;
x = 10;
EOF

cat <<'EOF' > should_fail/test3.alo
let x = 5;
let x = 6;
EOF

cat <<'EOF' > should_fail/test4.alo
let x = true;
let y = 5;
let z = x + y;
EOF

cat <<'EOF' > should_fail/test5.alo
let x = 5;
{
    let y = 10;
}
y = 3;
EOF

cat <<'EOF' > should_fail/test6.alo
let x = true;
let y = -x;
EOF

cat <<'EOF' > should_fail/test7.alo
let x = 5;
let y = !x + 2;
EOF

cat <<'EOF' > should_fail/test8.alo
let x = true;
let y = false;
let z = x < y;
EOF

cat <<'EOF' > should_fail/test9.alo
let x = 5;
let y = 10;
let z = x && y + 3;
EOF

cat <<'EOF' > should_fail/test10.alo
let x = 5;
let y = true;
x = y;
EOF

cat <<'EOF' > should_fail/test11.alo
let x = true;
if (x + 1) {
}
EOF

cat <<'EOF' > should_fail/test12.alo
let x = 5;
while (true + 1) {
}
EOF

cat <<'EOF' > should_fail/test13.alo
let x = 5;
{
    let x = true;
}
x = true;
EOF

cat <<'EOF' > should_fail/test14.alo
let x = true;
let y = true;
let z = x * y;
EOF

cat <<'EOF' > should_fail/test15.alo
let x = 5;
let y = x + unknown;
EOF

echo "Created 15 failing tests in should_fail/"
