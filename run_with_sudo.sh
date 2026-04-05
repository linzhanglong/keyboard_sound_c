#!/bin/bash
# 运行键盘发音程序（需要sudo权限）

echo "=== 键盘发音程序 ==="
echo ""
echo "程序需要访问键盘设备，需要以下权限之一："
echo "1. 使用 sudo 运行（推荐）"
echo "2. 将用户加入 input 组：sudo usermod -a -G input \$USER"
echo ""
echo "当前尝试使用 sudo 运行..."
echo ""

# 尝试运行程序
sudo ./keyboard_sound "$@"
