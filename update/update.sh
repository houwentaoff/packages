#!/bin/sh
#自动升级根文件系统 内核镜像的脚本 
#通过/proc/versionn中的信息  利用烧写工具对flash进行烧写 (/dev/mtd2)
filepath="/mnt/data/update/"
#filepath="/mnt/version-release/test"
path=$(pwd)
uImage="$path/vmlinux.ub"
rootfs="/$path/rootfs.img"
path_rootfs_ver="/version/version"
echo -e "\033[41;36m Warning: update will take up several minutes,please keep patience ......do not plug out power!!!!!! \033[0m "
chmod 777 $(pwd)
#tar xvf loragw*
unzip -o loragw*
#check file path Ok
if [ ! -d "$path" ];
then
    echo "file path not exist... update fail!!!exit"
    exit
fi
#check kernel image version
echo "check kernel image version......"
cat /proc/version  >/tmp/test.txt
sleep 1
cat /tmp/test.txt |while read line
do 
    kernel_version=`awk 'BEGIN {split("'"$line"'",arr);print arr[3]}'`
    echo -e "\033[40;36mkernel_version:\033[0m $kernel_version"
    cat version |while read line
do
    new_kernel_version=`awk 'BEGIN {split("'"$line"'",arr);print arr[5]}'`
    echo -e "\033[40;36mnew_kernel_version:\033[0m $new_kernel_version"
    if [ $kernel_version != $new_kernel_version ];
    then
        echo "version $kernel_version was old ....will update!"
        image_update_flag="TRUE"
        #check uImage file exist or not
        if [ ! -f "$uImage" ];
        then
            echo -e "\033[41;36m update file $uImage not exist,please put  file $uImage in Path $filepath \033[0m"
            exit
        else
            echo "file $uImage check Ok"
        fi
        #update uImage
        flash_erase /dev/mtd1 0 0
        echo "eraseall image partition success"
        nandwrite -q -p /dev/mtd1 $uImage
        echo "update kernel image success"
        rm /tmp/test.txt
    else
        echo "version $kernel_version is the latest..... skip update!"
        image_update_flag="FALSE"
        rm /tmp/test.txt
    fi
done
done
#check rootfs_path_version
if [  ! -f "$path_rootfs_ver" ] ;
then
    echo -e "\033[41;36m rootfs version does not exist, will be update rootfs \033[0m"
    #mkdir /version
    #touch /version/version
    #echo "original version" > $path_rootfs_ver
    line="original version"
fi
#check rootfs_version
echo echo `cat  /version/version || echo "$line"` |while read line
do
    rootfs_version=`awk 'BEGIN {split("'"$line"'",arr);print arr[3]}'`
    echo -e "\033[40;36mrootfs_version:\033[0m $rootfs_version"
    sleep 1
    cat version |while read line
do
    new_rootfs_version=`awk 'BEGIN {split("'"$line"'",arr);print arr[3]}'`
    echo -e "\033[40;36mnew_rootfs_version:\033[0m $new_rootfs_version"
    if [ "$rootfs_version" != "$new_rootfs_version" ];
    then
        echo "version $rootfs_version was old ....will update!"
        rootfs_update_flag="TRUE"
        #check rootfs file exist or not
        if [ ! -f "$rootfs" ];
        then
            echo -e "\033[41;36m update file rootfs not exist, please put file rootfs.img in path $filepath \033[0m"
            exit
        else
            echo "file rootfs check ok"
        fi
        flash_erase /dev/mtd2 0 0
        echo "earse partition rootfs success"
        nandwrite -q -p /dev/mtd2 rootfs.img
        echo "update file system success"
    else
        echo "version $rootfs_version is the latest..... skip update!"
        rootfs_update_flag="FALSE"
    fi
done
done
sleep 1
#rm -rf $uImage rootfs.img version
rm -rf /tmp/*
rm -rf loragw*.zip
echo "update done"

