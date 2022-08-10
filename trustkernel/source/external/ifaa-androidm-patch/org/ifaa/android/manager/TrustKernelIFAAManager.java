package org.ifaa.android.manager;


import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.util.Log;

import org.ifaa.android.manager.ta.TACommands;
import org.ifaa.android.manager.ta.TAInterationV1;
import org.ifaa.android.manager.util.ByteUtils;
import org.ifaa.android.manager.util.Result;

public class TrustKernelIFAAManager extends IFAAManager{

    public static String TAG = "TrustKernelIFAAManager";
    private Context ctx = null;
    private int authType = 0;

    private String devicemodel = null;
    private int biotypes = -1;

    public TrustKernelIFAAManager(Context ctx, int authType){
        this.ctx = ctx;
        this.authType = authType;
    }

    @Override
    public String getDeviceModel() {
        if(devicemodel == null) {
            Result result = TAInterationV1.sendCommand(this, ctx, TACommands.IFAA_ANDROID_GET_DEVICE_MODEL);
            if(result.getStatus() == Result.RESULT_SUCCESS) {
                devicemodel = new String(result.getData());
            }
        }
        Log.d(TAG, "getDeviceModel: " + devicemodel);
        return devicemodel;
    }

    @Override
    public int getSupportBIOTypes(Context context) {
        if(biotypes == -1) {
            Result result = TAInterationV1.sendCommand(this, context, TACommands.IFAA_ANDROID_GET_SUPPORT_BIO_TYPE);
            if(result.getStatus() == Result.RESULT_SUCCESS) {
                biotypes = ByteUtils.toInt(result.getData());
            }
        }
        Log.d(TAG, "getSupportBIOTypes: " + biotypes);
        return biotypes;
    }


    @Override
    public int getVersion() {
        return 1;
    }

    @Override
    public int startBIOManager(Context context, int authType) {
        Log.d(TAG, "startBIOManager");
        Intent intent = new Intent(Settings.ACTION_SECURITY_SETTINGS);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(intent);
        return 0;
    }

    static {
        try {
            System.loadLibrary("teeclientjni");
        }catch(UnsatisfiedLinkError e){
            Log.e(TAG, "Unable to load libteeclientjni.so. Dit you forget to upload to /system/lib64 and /system/lib ?");
            e.printStackTrace();
        }
    }

    /*
    @Override
    public String getDeviceModel() {
        Log.d(TAG, "getDeviceModel");
        String model;
        Cursor cr;
        if(ctx == null) {
            Log.e(TAG, "getDeviceModel with built-in model");
            return Build.MODEL.replace(" ", "-");
        }
        cr = ctx.getContentResolver().query(Uri.parse("content://trustkernel.authentication.information"), null, null, null, null);
        if(cr != null && cr.moveToFirst()) {
            model = cr.getString(cr.getColumnIndex("DeviceModel"));
            Log.d(TAG, "device model found, its: " + model);
            return model;
        }else {
            if(cr == null) {
                Log.d(TAG, "device model not initialized");
            }
        }
        Log.d(TAG, "device model not set, use default model:" + Build.MODEL.replace(" ", "-"));
        return Build.MODEL.replace(" ", "-");
    }
    */
    /*
    @Override
    public int getSupportBIOTypes(Context context) {
        Log.d(TAG, "getSupportBIOTypes");
        int res = 0;
        //currently we only support Fingerprint
        try {
            FingerprintManager fpman = (FingerprintManager) context.getSystemService(Context.FINGERPRINT_SERVICE);
            if (fpman != null && fpman.isHardwareDetected()) {
                res |= 1;
            }
        }catch(SecurityException e) {

        }

        return res;
    }
    */

    /*
    @Override
    public int getVersion() {
        Log.d(TAG, "getVersion");
        int version = 1;
        Cursor cr;
        if(ctx == null) {
            Log.e(TAG, "getVersion with built-in value 1");
            return version;
        }
        cr = ctx.getContentResolver().query(Uri.parse("content://trustkernel.authentication.information"), null, null, null, null);
        if(cr != null && cr.moveToFirst()) {
            version = cr.getInt(cr.getColumnIndex("IFAAManagerVersion"));
            Log.d(TAG, "getVersion found, its: " + version);
            return version;
        }else {
            if(cr == null) {
                Log.d(TAG, "device model not initialized");
            }
        }
        Log.d(TAG, "IFAAManager version not set, use default version:" + 1);
        return version;
    }
    */

}
