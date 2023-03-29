obj-$(CONFIG_OASES_SIGNATURE_SUPPORT) += \
	asn1_decoder.o \
	generic_mpih-add1.o \
	generic_mpih-lshift.o \
	generic_mpih-mul1.o \
	generic_mpih-mul2.o \
	generic_mpih-mul3.o \
	generic_mpih-rshift.o \
	generic_mpih-sub1.o \
	mpi-bit.o \
	mpi-cmp.o \
	mpicoder.o \
	mpih-cmp.o \
	mpih-div.o \
	mpih-mul.o \
	mpi-pow.o \
	mpiutil.o \
	oid_registry.o \
	public_key.o \
	rsa.o \
	sha512_generic.o \
	x509-asn1.o \
	x509_cert_parser.o \
	x509_rsakey-asn1.o

$(obj)/oid_registry.o: $(src)/oid_registry_data.c
$(obj)/x509_cert_parser.o: $(src)/x509-asn1.h $(src)/x509_rsakey-asn1.h
$(obj)/x509-asn1.o: $(src)/x509-asn1.c $(src)/x509-asn1.h
$(obj)/x509_rsakey-asn1.o: $(src)/x509_rsakey-asn1.c $(src)/x509_rsakey-asn1.h