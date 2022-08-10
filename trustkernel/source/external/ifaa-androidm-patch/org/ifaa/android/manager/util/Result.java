package org.ifaa.android.manager.util;


public class Result {

    public static final int RESULT_FAIL = -1;
    public static final int RESULT_SUCCESS = 0;
    public static String TAG;
    protected byte[] data;
    protected int status;

    static {
        TAG = Result.class.getName();
    }
    public Result() {
        this.status = -1;
    }
    public Result(int status, byte[] data) {
        this.status = -1;
        this.status = status;
        this.data = data;
    }
    public int getStatus() {
        return this.status;
    }
    public void setStatus(int status) {
        this.status = status;
    }
    public byte[] getData() {
        return this.data;
    }
    public void setData(byte[] data) {
        this.data = data;
    }
}
