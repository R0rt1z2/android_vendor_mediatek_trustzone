#!/usr/bin/env python
#
# Copyright (c) 2015, Linaro Limited
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

def get_args():
	from argparse import ArgumentParser

	parser = ArgumentParser()
	parser.add_argument('--in', required=True, dest='inf', help='Name of in file')
	parser.add_argument('--key', required=False, help='Name of key file')
	parser.add_argument('--passphrase', required=False, help='Passphrase of key file')
	parser.add_argument('--project_cert', required=False, help='Name of project certificate')
	parser.add_argument('--silent', required=False, action='store_true')
	return parser.parse_args()

def memcmp(a,b):
	if len(a) < len(b):
		return -1
	elif len(a) > len(b):
		return 1
	for i in xrange(0,len(a)):
		if a[i] != b[i]:
			if a[i] < b[i]:
				return -1
			else:
				return 1
	return 0

def parse_project_pubkey(f):
	from Crypto.Signature import PKCS1_v1_5
	from Crypto.Hash import SHA256
	from Crypto.PublicKey import RSA
	import struct

	ilen = 4
	CERT_MAGIC = "PROJECT_VERIFIE\0"

	cm = f.read(len(CERT_MAGIC))
	if cm != CERT_MAGIC:
		return None

	data_size, sig_size, digest_size = struct.unpack('<III', f.read(ilen * 3))

	# skip sig size/digest_size 
	f.read(sig_size + digest_size)

	nbmp, = struct.unpack('<I', f.read(ilen))

	if nbmp == 0:
		return None
    
	for x in xrange(nbmp):
		# skip brand
		l, = struct.unpack('<I', f.read(ilen))
		f.read(l)
		# skip model
		l, = struct.unpack('<I', f.read(ilen))
		f.read(l)
		# skip platform
		l, = struct.unpack('<I', f.read(ilen))
		f.read(l)

	e, = struct.unpack("<I", f.read(ilen))
	modulus_size, = struct.unpack('<I', f.read(ilen))
	modulus = f.read(modulus_size)
	if len(modulus) != modulus_size:
		return None
	e = long(e)
	m = long(modulus.encode('hex'), 16)
	return RSA.construct((m, e))

def main():
	SHDR_TA = 0
	SHDR_SP_TA = 1
	SHDR_VENDOR_TA = 2

	from Crypto.Signature import PKCS1_v1_5
	from Crypto.Hash import SHA256
	from Crypto.PublicKey import RSA
	import struct

	args = get_args()

	f = open(args.inf, 'rb')
	img = f.read()
	f.close()

	key = None

	if args.key == None and args.project_cert == None:
		if not args.silent:
			print "Must provide one of key/project cert"
		return -1

	if args.key != None and args.project_cert != None:
		if not args.silent:
			print "Cannot provide key/project cert simultaneously"
		return -1

	if args.key != None:
		f = open(args.key, 'rb')
		key = RSA.importKey(f.read(), args.passphrase)
		f.close()

	if args.project_cert != None:
		f = open(args.project_cert, 'rb')
		key = parse_project_pubkey(f)
		f.close()
		if key is None:
			print "Invalid project certificate: " + args.project_cert
			return -11

	signer = PKCS1_v1_5.new(key)
	h = SHA256.new()

	magic, img_type, img_size, algo, digest_len, sig_len = struct.unpack('<IIIIHH', img[0:20])

	if magic != 0x4f545348:
		if not args.silent:
			print "Invalid TA magic"
		return -2

	if img_type != SHDR_TA and img_type != SHDR_SP_TA and img_type != SHDR_VENDOR_TA:
		if not args.silent:
			print 'Invalid TA type: %d' % (img_type)
		return -3

	if img_type == SHDR_SP_TA:
		if not args.silent:
			print "SP TA not supported to verify"
		return -4

	if algo != 0x70004830:
		if not args.silent:
			print "Unsupported algorithm: 0x%x" % (algo)
		return -5

	if digest_len != h.digest_size or\
		sig_len != 256 or\
		img_size != len(img) - 20 - digest_len - sig_len:
		if not args.silent:
			print "Invalid digest_len/sig_len/img_size"
		return -6

	if args.project_cert != None and img_type != SHDR_VENDOR_TA:
		if not args.silent:
			print "SYSTEM TA cannot be verified by project certificate"
		return -7

	if args.project_cert == None and img_type == SHDR_VENDOR_TA:
		if not args.silent:
			print "VENDOR TA must be verified by project certificate"
		return -8

	h.update(img[0:20])
	h.update(img[(20+digest_len+sig_len):])
	if memcmp(h.digest(), img[20:(20+digest_len)]) != 0:
		if not args.silent:
			print "Hash mismatch"
		return -9

	if signer.verify(h, img[(20+digest_len):(20+digest_len+sig_len)]):
		if args.silent:
			print "VALIDTA"
		else:
			print "TA is verified"
	else:
		print "TA cannot be verified"

if __name__ == "__main__":
	main()
