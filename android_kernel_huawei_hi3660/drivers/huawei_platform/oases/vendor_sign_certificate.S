	.section ".data","aw"

	.global vendor_sign_certificate_list
	.global vendor_sign_certificate_list_end

vendor_sign_certificate_list:
	.incbin "vendor_key.x509"
vendor_sign_certificate_list_end:
