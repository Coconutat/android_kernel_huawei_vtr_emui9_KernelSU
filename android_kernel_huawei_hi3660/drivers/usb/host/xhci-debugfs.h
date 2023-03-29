

#ifdef CONFIG_HISI_DEBUG_FS
int xhci_create_debug_file(struct xhci_hcd *xhci);
void xhci_remove_debug_file(struct xhci_hcd *xhci);
#else
static inline int xhci_create_debug_file(struct xhci_hcd *xhci)
{
	return 0;
}

static inline void xhci_remove_debug_file(struct xhci_hcd *xhci) { }
#endif

