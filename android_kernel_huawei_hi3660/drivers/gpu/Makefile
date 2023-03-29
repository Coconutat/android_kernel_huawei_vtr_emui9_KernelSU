# drm/tegra depends on host1x, so if both drivers are built-in care must be
# taken to initialize them in the correct order. Link order is the only way
# to ensure this currently.
obj-y	  += drm/ host1x/ ipu-v3/ vga/
obj-$(CONFIG_GPU_GMC_GENERIC) += gmc/
obj-$(CONFIG_MALI_MIDGARD)    += arm/
