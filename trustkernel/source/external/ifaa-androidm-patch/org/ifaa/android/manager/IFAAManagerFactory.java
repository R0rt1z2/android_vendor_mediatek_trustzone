package org.ifaa.android.manager;


import android.content.Context;

public class IFAAManagerFactory {

    private static IFAAManager instance = null;

    public static IFAAManager getIFAAManager(Context ctx, int authType) {
        synchronized (IFAAManagerFactory.class) {
            if (instance == null) {
                instance = new TrustKernelIFAAManager(ctx, authType);
            }
            return instance;
        }
    }
}
