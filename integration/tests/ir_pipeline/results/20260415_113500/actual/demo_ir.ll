; module: demo_ir_pipeline
define i32 @inc(i32 %v.in) {
entry:
  %v.addr = alloca i32
  %t.addr = alloca i32
  %t1.addr = alloca i32
  store i32 %v.in, i32* %v.addr
  br label %bb_1

bb_1:
  %r1 = load i32, i32* %v.addr
  %r2 = add nsw i32 %r1, 1
  store i32 %r2, i32* %t1.addr
  %r3 = load i32, i32* %t1.addr
  store i32 %r3, i32* %t.addr
  %r4 = load i32, i32* %t.addr
  ret i32 %r4
}

define i32 @main() {
entry:
  %x.addr = alloca i32
  %y.addr = alloca i32
  %limit.addr = alloca i32
  %result.addr = alloca i32
  %t2.addr = alloca i32
  %t3.addr = alloca i32
  %t4.addr = alloca i32
  %t5.addr = alloca i32
  %t6.addr = alloca i32
  br label %bb_4

bb_4:
  store i32 1, i32* %x.addr
  store i32 2, i32* %y.addr
  store i32 20, i32* %limit.addr
  store i32 0, i32* %result.addr
  br label %bb_8

bb_8:
  %r1 = load i32, i32* %x.addr
  %r2 = icmp slt i32 %r1, 8
  br i1 %r2, label %bb_10, label %bb_9

bb_9:
  br label %bb_23

bb_10:
  %r3 = load i32, i32* %y.addr
  %r4 = call i32 @inc(i32 %r3)
  store i32 %r4, i32* %t2.addr
  %r5 = load i32, i32* %t2.addr
  store i32 %r5, i32* %y.addr
  %r6 = load i32, i32* %y.addr
  %r7 = mul nsw i32 %r6, 2
  store i32 %r7, i32* %t3.addr
  %r8 = load i32, i32* %x.addr
  %r9 = load i32, i32* %t3.addr
  %r10 = add nsw i32 %r8, %r9
  store i32 %r10, i32* %t4.addr
  %r11 = load i32, i32* %t4.addr
  store i32 %r11, i32* %result.addr
  %r12 = load i32, i32* %result.addr
  %r13 = load i32, i32* %limit.addr
  %r14 = icmp sgt i32 %r12, %r13
  br i1 %r14, label %bb_17, label %bb_16

bb_16:
  br label %bb_20

bb_17:
  %r15 = load i32, i32* %result.addr
  %r16 = sub nsw i32 %r15, 3
  store i32 %r16, i32* %t5.addr
  %r17 = load i32, i32* %t5.addr
  store i32 %r17, i32* %x.addr
  br label %bb_22

bb_20:
  %r18 = load i32, i32* %result.addr
  %r19 = add nsw i32 %r18, 1
  store i32 %r19, i32* %t6.addr
  %r20 = load i32, i32* %t6.addr
  store i32 %r20, i32* %x.addr
  br label %bb_22

bb_22:
  br label %bb_8

bb_23:
  %r21 = load i32, i32* %x.addr
  ret i32 %r21
}

