; ModuleID = 'alo_module'
source_filename = "alo_module"

@fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define void @main() {
entry:
  %x = alloca i32, align 4
  store i32 0, ptr %x, align 4
  br label %whilecond

whilecond:                                        ; preds = %whilebody, %entry
  %x1 = load i32, ptr %x, align 4
  %netmp = icmp ne i32 %x1, 5
  br i1 %netmp, label %whilebody, label %whileend

whilebody:                                        ; preds = %whilecond
  %x2 = load i32, ptr %x, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @fmt, i32 %x2)
  %x3 = load i32, ptr %x, align 4
  %addtmp = add i32 %x3, 1
  store i32 %addtmp, ptr %x, align 4
  br label %whilecond

whileend:                                         ; preds = %whilecond
  ret void
}

declare i32 @printf(ptr, ...)
