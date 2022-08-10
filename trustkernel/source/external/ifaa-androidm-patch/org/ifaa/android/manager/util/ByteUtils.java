package org.ifaa.android.manager.util;

import java.math.BigInteger;

public class ByteUtils {
    protected static final char[] HEX_ARRAY;
    public static final String HEX_DIGITS = "0123456789ABCDEF";
    public static final int ACTION_MASK = 0xff;

    static {
        HEX_ARRAY = HEX_DIGITS.toCharArray();
    }

    public static String toHexString(byte[] bytes) {
        char[] hexChars = new char[(bytes.length * 2)];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & ACTION_MASK;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[(j * 2) + 1] = HEX_ARRAY[v & 15];
        }
        return new String(hexChars);
    }

    public static byte[] hexToBytes(String hexString) {
        if ((hexString.length() & 1) != 0) {
            hexString = "0" + hexString;
        }
        hexString = hexString.toUpperCase();
        int length = hexString.length() / 2;
        char[] hexChars = hexString.toCharArray();
        byte[] result = new byte[length];
        for (int i = 0; i < length; i++) {
            int pos = i * 2;
            result[i] = (byte) ((hexChartoByte(hexChars[pos]) << 4) | hexChartoByte(hexChars[pos + 1]));
        }
        return result;
    }

    private static byte hexChartoByte(char c) {
        return (byte) HEX_DIGITS.indexOf(c);
    }

    public static int toInt(byte[] input) {
        return toInt(input, 0);
    }

    public static int toInt(byte[] input, int offset) {
        return ((((input[offset + 3] & ACTION_MASK) << 24) + ((input[offset + 2] & ACTION_MASK) << 16)) + ((input[offset + 1] & ACTION_MASK) << 8)) + (input[offset + 0] & ACTION_MASK);
    }

    public static int toUInt16(byte[] input) {
        return toUInt16(input, 0);
    }

    public static int toUInt16(byte[] input, int offset) {
        return ((input[offset + 1] & ACTION_MASK) << 8) + (input[offset + 0] & ACTION_MASK);
    }

    public static void writeUInt16(int input, byte[] dest, int offset) {
        dest[offset + 0] = (byte) (input & ACTION_MASK);
        dest[offset + 1] = (byte) ((input >> 8) & ACTION_MASK);
    }

    public static void write(int input, byte[] dest, int offset) {
        dest[offset + 0] = (byte) (input & ACTION_MASK);
        dest[offset + 1] = (byte) ((input >> 8) & ACTION_MASK);
        dest[offset + 2] = (byte) ((input >> 16) & ACTION_MASK);
        dest[offset + 3] = (byte) ((input >> 24) & ACTION_MASK);
    }

    public static void write(byte[] input, int length, byte[] dest, int offset) {
        for (int i = 0; i < length; i++) {
            dest[offset + i] = input[i];
        }
    }

    public static void copy(byte[] input, int inputOffset, int length, byte[] dest, int destOffset) {
        for (int i = 0; i < length; i++) {
            dest[destOffset + i] = input[inputOffset + i];
        }
    }

    public static byte[] toBytes(int idx) {
        byte[] result = new byte[4];
        write(idx, result, 0);
        return result;
    }

    public static BigInteger toBigInteger(byte[] data, int offset, int length) {
        byte[] bigIntegerData = new byte[length];
        write(data, length, bigIntegerData, offset);
        return new BigInteger(bigIntegerData);
    }

    public static BigInteger toBigInteger(byte[] data) {
        return new BigInteger(data);
    }
}
