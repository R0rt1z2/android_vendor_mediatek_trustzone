package org.ifaa.android.manager.ta;

import android.content.Context;


import android.content.Context;
import android.util.Log;

import org.ifaa.android.manager.IFAAManager;
import org.ifaa.android.manager.util.PackageUtils;
import org.ifaa.android.manager.util.Result;
import org.ifaa.android.manager.util.ByteUtils;

public class TAInterationV1 {

    public static byte[] getHeader(Context context) {
        try {
            byte[] packageSign = PackageUtils.getApkKeyHashBytes(context, context.getPackageName());
            byte[] packageName = context.getPackageName().getBytes();
            int packageSignLength = packageSign.length;
            int packageNameLength = packageName.length;
            byte[] param = new byte[(((packageSignLength + 8) + 4) + packageNameLength)];
            System.arraycopy(ByteUtils.toBytes(1), 0, param, 0, 4);
            int startPoint = 0 + 4;
            System.arraycopy(ByteUtils.toBytes(packageSignLength), 0, param, startPoint, 4);
            System.arraycopy(packageSign, 0, param, startPoint + 4, packageSignLength);
            startPoint = packageSignLength + 8;
            System.arraycopy(ByteUtils.toBytes(packageNameLength), 0, param, startPoint, 4);
            startPoint += 4;
            System.arraycopy(packageName, 0, param, startPoint, packageNameLength);
            startPoint += packageNameLength;
            return param;
        } catch (Exception e) {
            return null;
        }
    }

    public static Result sendCommand(IFAAManager manager, Context context, int command) {
        byte[] header = getHeader(context);
        if (header == null || header.length == 0) {
            return new Result(Result.RESULT_FAIL, null);
        }
        int headerLength = header.length;
        byte[] param = new byte[(headerLength + 4)];
        System.arraycopy(header, 0, param, 0, headerLength);
        int startPoint = 0 + headerLength;
        System.arraycopy(ByteUtils.toBytes(command), 0, param, startPoint, 4);
        startPoint += 4;
        return buildResult(doSendData(manager, context, param));
    }

    public static Result sendCommandAndData(IFAAManager manager, Context context, int command, byte[] data) {
        if (data == null || data.length == 0) {
            return sendCommand(manager, context, command);
        }
        byte[] header = getHeader(context);
        if (header == null || header.length == 0) {
            return new Result(Result.RESULT_FAIL, null);
        }
        int headerLength = header.length;
        int dataLength = data.length;
        byte[] param = new byte[(((headerLength + 4) + 4) + dataLength)];
        System.arraycopy(header, 0, param, 0, headerLength);
        int startPoint = 0 + headerLength;
        System.arraycopy(ByteUtils.toBytes(command), 0, param, startPoint, 4);
        startPoint += 4;
        System.arraycopy(ByteUtils.toBytes(dataLength), 0, param, startPoint, 4);
        startPoint += 4;
        System.arraycopy(data, 0, param, startPoint, dataLength);
        startPoint += dataLength;
        return buildResult(doSendData(manager, context, param));
    }

    private static byte[] doSendData(IFAAManager manager, Context context, byte[] data) {
        return manager.processCmd(context, data);
    }

    private static Result buildResult(byte[] resultData) {
        if (resultData == null || resultData.length < 8) {
            return new Result(Result.RESULT_FAIL, null);
        }
        byte[] data = new byte[(resultData.length - 8)];
        ByteUtils.copy(resultData, 8, resultData.length - 8, data, 0);
        Result result = new Result();
        result.setStatus(ByteUtils.toInt(resultData));
        result.setData(data);
        Log.d("TrustKernel", "result:" + result.getStatus());
        return result;
    }
}
