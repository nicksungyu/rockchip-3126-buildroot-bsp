#!/bin/sh

GAME_DIR=$PWD/../../../../board/rockchip/rk312x/fs-overlay/game

echo ">>>>>>>>>>> game update >>>>>>>>>>>>>>>>"
j=`awk '/# Launch Vortex/{ print NR; exit }' ${GAME_DIR}/vortex.sh`
echo "Line number=${j}"
strA=`grep "/tmp/game_cmdline" ${GAME_DIR}/vortex.sh | wc -L`
if [ "$strA" -ne "30" ];then 
	sed -i "${j} i echo \$game > /tmp/game_cmdline"  ${GAME_DIR}/vortex.sh
fi
