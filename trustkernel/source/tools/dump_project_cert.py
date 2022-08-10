#!/usr/bin/env python
#
# Copyright (c) 2016, Trustkernel co.Ltd
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

import sys
import os
import struct

from Crypto.PublicKey import RSA

def get_args():
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument('--cert', required=True, help='Name of cert')
    parser.add_argument('--type', required=False, help='Name of cert')
    parser.add_argument('--silent', required=False, action='store_true')


    return parser.parse_args()

# 4 byte is reserved for revokeid

CERT_SIZE_LIMIT = 1024 - 4
CERT_MAGIC = "PROJECT_VERIFIE\0"

def extract_pubkey(pem):
    import array
    from Crypto.Util.number import long_to_bytes

    f = open(pem, 'r')
    key = RSA.importKey(f.read())
    f.close()

    arr = array.array("B", long_to_bytes(key.publickey().n))

    dat = struct.pack("<I", key.publickey().e) + struct.pack("<I", len(arr))

    for b in arr:
        dat += chr(b)

    return dat

def certtype_i2s(i):
    if i == "\1":
        return "EVAL"
    elif i == "\2":
        return "FULL"
    elif i == "\3":
        return "LIMITED"
    elif i == "\4":
        return "REVOKE"
    elif i == "\5":
        return "BUILTIN"
    elif i == "\6":
        return "VOLUME"
    else:
        print "Invalid cert type: " + i
        sys.exit(-3)

def certtype_s2i(s):
    if s == "evaluate":
        return "\1"
    elif s == "full":
        return "\2"
    elif s == "limited":
        return "\3"
    elif s == "revoke":
        return "\4"
    elif s == "builtin":
        return "\5"
    elif s == "volume":
        return "\6"
    else:
        return None

def main():
    from Crypto.Signature import PKCS1_v1_5
    from Crypto.Hash import SHA256

    args = get_args()

    project_pub = RSA.importKey("""-----BEGIN PUBLIC KEY-----
    MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvtq/n5mqN276xQkgF731
    yLYGbltp4q0ciB9jBf7wonPdtN3OLA6aLRef8sxNe4Dj39lPdZ/KcjWOcuss88mI
    GlcsrWzqwLYQEeL1BFfx/3gYhSmul8XgpL7CG+iTGWFLP9+qebYJyrupKvndMtji
    NzZhPGpPhweC0+j0ngFVssdtGIW6CwflK92ONNSSAGQiMEPFbHo1Ca5eB0ndrI7n
    2TA60txJoicilBugeAee52Y/QdxNXVIKh6e0CtORYvVp+whxdZkQe5Exsno6fUyV
    80LJtQk8IltJVxJyBi2csO7dZfl+SAgrfvd6NifhOUrg/xYf9LMWkA9tPcPleZ9e
    eQIDAQAB
    -----END PUBLIC KEY-----""")

    s = PKCS1_v1_5.new(project_pub)

    f = open(args.cert, 'rb')
    cert = f.read()
    f.close()

    magic = cert[:len(CERT_MAGIC)]
    if magic != CERT_MAGIC:
        if not args.silent:
            print "Invalid magic: %s" % (magic)
        return -1

    hdr1_size = len(struct.pack('<III', 0, 0, 0))

    hdr = CERT_MAGIC + cert[len(CERT_MAGIC):len(CERT_MAGIC) + hdr1_size]

    data_size, sig_size, digest_size = \
        struct.unpack('<III', cert[len(CERT_MAGIC):len(CERT_MAGIC) + hdr1_size])

    cert = cert[len(CERT_MAGIC) + hdr1_size:]

    if data_size + sig_size + digest_size + len(CERT_MAGIC) + hdr1_size > CERT_SIZE_LIMIT:
        if not args.silent:
            print "Invalid hdr: %d" %(data_size + sig_size + digest_size + len(CERT_MAGIC) + hdr1_size)
        return -2

    digest = cert[:digest_size]
    sig = cert[digest_size: digest_size + sig_size]
    data = cert[digest_size + sig_size:digest_size + sig_size + data_size]

    h = SHA256.new()
    h.update(hdr)
    h.update(data)
    if not s.verify(h, sig):
        if not args.silent:
            print "CERT HASH INVALID!!"
        return -4

    if h.digest() != digest:
        if not args.silent:
            print "CERT DIGEST INVALID!!"
        return -5

    # verify here
    ilen = len(struct.pack('<I', 0))
    nbmp, = struct.unpack('<I', data[: ilen])
    data = data[ilen:]

    if nbmp == 0 and args.silent:
        return -6

    if not args.silent:
        print "CERT PROJECTS: %d PROJECTS" % (nbmp)

    for x in xrange(nbmp):
        l, = struct.unpack('<I', data[: ilen])
        brand = data[ilen: ilen + l]
        data = data[ilen + l: ]

        l, = struct.unpack('<I', data[: ilen])
        model = data[ilen : ilen + l]
        data = data[ilen + l: ]

        l, = struct.unpack('<I', data[: ilen])
        platform = data[ilen : ilen + l]
        data = data[ilen + l: ]

        if not args.silent:
            print "\tBRAND: %s MODEL: %s PLATFORM:%s" % (brand, model, platform)

    # skip exponent
    exponent, = struct.unpack("<I", data[: ilen])
    data = data[ilen:]
    modulus_size, = struct.unpack('<I', data[: ilen])
    # skip modulus
    data = data[ilen + modulus_size:]
    if not args.silent:
        print "CERT VENDOR_KEY: [EXPONENT: %d Modulus Size: %u]" % (exponent, modulus_size)
    # skip modulus
    ctype, = struct.unpack('<c', data[:1])
    ctype_s = certtype_i2s(ctype)

    if args.type != None:
        expect_ctype_i_arr = args.type.split(",")
        find = False
        for i in xrange(len(expect_ctype_i_arr)):
            expect_ctype_i = certtype_s2i(expect_ctype_i_arr[i])
            if expect_ctype_i == ctype:
                find = True
                break
        if find == False:
            if not args.silent:
                print "Unexpected cert type %d" % (ord(ctype))
            return -7

    if ctype_s == "REVOKE":
        revokeid, = struct.unpack("<I", data[1:1 + ilen])
        if not args.silent:
            print "CERT TYPE: %s REVOKEID: %d" % (ctype_s, revokeid)
    else:
        if not args.silent:
            print "CERT TYPE: %s" % (ctype_s)

    if args.silent:
        print "VALIDCERT"

if __name__ == "__main__":
    sys.exit(main())
