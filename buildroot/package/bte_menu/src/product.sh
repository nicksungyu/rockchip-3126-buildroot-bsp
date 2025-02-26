#!/bin/sh

echo ">>>>>>>>> product shell script <<<<<<<<<"
PWD_DIR=$PWD # /home/share1/nick/rk3126/pacmania/buildroot/output/rockchip_rk312x/build/bte_menu
TARGET_OUTPUT_DIR=$PWD/../.. # /home/share1/nick/rk3126/pacmania/buildroot/output/rockchip_rk312x
GAME_DIR=$PWD/../../../../board/rockchip/rk312x/fs-overlay/game

MODEL=`grep 'model' $PWD/product | awk '{print $2}'`
touch $TARGET_OUTPUT_DIR/target/etc/model
echo "Project Number : $MODEL" > $TARGET_OUTPUT_DIR/target/etc/model;

VN=`grep 'Date Code' $GAME_DIR/version | awk '{print $4}'`
touch $TARGET_OUTPUT_DIR/target/etc/vn
echo "Date Code : $VN"  > $TARGET_OUTPUT_DIR/target/etc/vn;

PCBA=`grep 'pcba' $PWD/product | awk '{print $2}'`
touch $TARGET_OUTPUT_DIR/target/etc/pcba
echo "PCBA Version : $PCBA" > $TARGET_OUTPUT_DIR/target/etc/pcba;
