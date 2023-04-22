#!/bin/bash

# generate new link vmlinux
topdir=$(cd ${BALONG_TOPDIR}/../..; pwd)
kernel_sys_dir=${OBB_PRODUCT_DELIVERY_DIR}/image/kernel_system
kernel_out_dir=${kernel_sys_dir}${objtree##${topdir}}
kernel_src_dir=${kernel_sys_dir}${srctree##${topdir}}
mkdir -p ${kernel_out_dir}
mkdir -p ${kernel_src_dir}

cd ${objtree}
cp -rf $@ ${kernel_out_dir} --parents
cp -rf .config ${kernel_out_dir}
cp -rf scripts ${kernel_out_dir}
cp -rf include ${kernel_out_dir}
cp -rf arch/${SRCARCH}/include ${kernel_out_dir} --parents

cd ${srctree}
cp -rf scripts ${kernel_src_dir}
cp -rfL include ${kernel_src_dir}
cp -rf fs ${kernel_src_dir}
cp -rf init ${kernel_src_dir}
cp -rf drivers/md ${kernel_src_dir} --parents
cp -rf arch/${SRCARCH}/boot/Makefile ${kernel_src_dir} --parents
cp -rf arch/${SRCARCH}/boot/dts/Makefile ${kernel_src_dir} --parents
cp -rf arch/arm64/include ${kernel_src_dir} --parents
cp -rf arch/arm/include ${kernel_src_dir} --parents

if [ "${singleap}"x != "true"x ]; then
export KBUILD_VMLINUX_MAIN="${KBUILD_VMLINUX_MAIN}  drivers/hisi/modem/built-in.o"
fi

echo "#!/bin/bash" > ${kernel_out_dir}/link-vmlinux-new.sh
printenv | sed "s/^/export &/g;s/=/='/;s/$/&'/g" >> ${kernel_out_dir}/link-vmlinux-new.sh
sed -i "/CCACHE_DIR=/d" ${kernel_out_dir}/link-vmlinux-new.sh
echo "${CONFIG_SHELL} ${srctree}/scripts/link-vmlinux.sh ${LD} ${LDFLAGS} ${LDFLAGS_vmlinux}" >> ${kernel_out_dir}/link-vmlinux-new.sh
echo "${MAKE} -f ${srctree}/scripts/Makefile.build obj=arch/${SRCARCH}/boot arch/${SRCARCH}/boot/${KBUILD_IMAGE}" >> ${kernel_out_dir}/link-vmlinux-new.sh
echo "cp -pf ${objtree}/arch/${SRCARCH}/boot/Image ${OBB_PRODUCT_DELIVERY_DIR}/image/kernel.uncompressed" >> ${kernel_out_dir}/link-vmlinux-new.sh
echo "cp -pf ${objtree}/arch/${SRCARCH}/boot/${KBUILD_IMAGE} ${ANDROID_PRODUCT_OUT}/kernel" >> ${kernel_out_dir}/link-vmlinux-new.sh
echo "cp -pf ${ANDROID_PRODUCT_OUT}/kernel ${OBB_PRODUCT_DELIVERY_DIR}/image/" >> ${kernel_out_dir}/link-vmlinux-new.sh
echo "cp -pf ${objtree}/vmlinux ${OBB_PRODUCT_DELIVERY_DIR}/lib/" >> ${kernel_out_dir}/link-vmlinux-new.sh
echo -n ${topdir} > ${kernel_sys_dir}/topdir.txt
chmod 777 ${kernel_out_dir}/link-vmlinux-new.sh
