	.section ".data","aw"

	.global oases_sign_certificate_list
	.global oases_sign_certificate_list_end

oases_sign_certificate_list:
	.incbin "oases_key.x509"
oases_sign_certificate_list_end:
