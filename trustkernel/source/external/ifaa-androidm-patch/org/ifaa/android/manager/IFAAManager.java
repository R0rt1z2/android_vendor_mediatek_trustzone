package org.ifaa.android.manager;

import android.content.Context;

/*
 *IFAAManager为抽象类，需要放到系统framework中，通过IFAAManager生成实例。
 */
public abstract class IFAAManager
{


	/*
	 返回系统手机型号，model字段用于标记手机的型号，主要是指的一个系列。例如一个
	 厂商的各个MODLE，例如三星的S6有可能叫SAMSUNG-SM9200， SAMSUNG-SM9201等等，其实都是指的一个
	 系列，但是他们都是使用的同一个方案，例如TA和芯片和指纹芯片。所以对model的定义就是一个系列，
	 他们对应了一个内置的厂商TA私钥。
	 *
	 */
    public abstract String getDeviceModel();

	/*
	 返回手机上支持的校验方式，目前IFAF协议1.0版本指纹为0x01、虹膜为0x02，验证类
	 型可以为多种不同方式组合，用‘或’操作符拼接。
	 */
    public abstract int getSupportBIOTypes(Context context);

	/*
	 *返回Manager接口版本，目前为1。
	 */
    public abstract int getVersion();

	/*
	 *功能描述：通过ifaateeclient的so文件实现REE到TAA的通道。
	 参数描述：byte[] param 用于传输到IFAA TA的数据buffer。
	 返回值：byte[] 返回IFAA TA返回REE的数据buffer。
	 */
    public native byte[] processCmd(Context context, byte[] param);

	/*
	 *
	 启动系统的指纹管理应用界面，让用户进行指纹录入。指纹录入是在系统的指纹管理应
	 用中实现的，本函数的作用只是将指纹管理应用运行起来，直接进行页面跳转，方便用户录入。
	 参数描述：context:上下文环境
	 authType: 生物特征识别类型，指纹为1，虹膜为2
	 返回值：0，成功启动指纹管理应用；其他值，启动指纹管理应用失败。
	 返回值 取值 说明
	 COMMAND_OK 0 成功
	 COMMAND_FAIL -1 失败
	 */
    public abstract int startBIOManager(Context context, int authType);
}
