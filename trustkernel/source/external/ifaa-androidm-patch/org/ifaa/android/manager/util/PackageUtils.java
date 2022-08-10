package org.ifaa.android.manager.util;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import java.io.ByteArrayInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;

public class PackageUtils {
    public static byte[] getApkKeyHashBytes(Context context, String appName) throws NameNotFoundException, CertificateException, NoSuchAlgorithmException {
        byte[] certData = CertificateFactory.getInstance("X509").generateCertificate(
                new ByteArrayInputStream(context.getPackageManager().getPackageInfo(appName, PackageManager.GET_SIGNATURES).signatures[0].toByteArray())).getEncoded();
        MessageDigest messageDigest = MessageDigest.getInstance("SHA-1");
        messageDigest.update(certData);
        return messageDigest.digest();
    }
}
