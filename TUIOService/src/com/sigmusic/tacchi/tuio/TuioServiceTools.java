package com.sigmusic.tacchi.tuio;

import java.util.List;

import android.app.ActivityManager;
import android.app.Service;
import android.content.Context;
import android.util.Log;

public class TuioServiceTools {
	public final static String TAG = "TUIOServiceTools";
    public static boolean isServiceRunning(Context context){
        final ActivityManager activityManager = (ActivityManager)context.getSystemService(context.ACTIVITY_SERVICE);
        final List<ActivityManager.RunningServiceInfo> services = activityManager.getRunningServices(Integer.MAX_VALUE);
        
        String tuioclassname = TuioService.class.getName();
        
	    for (int i = 0; i < services.size(); i++) {
	    	String classname = services.get(i).service.getClassName();
	        Log.d(TAG, "class   name: " + classname);
	        Log.d(TAG, "Service name: " + tuioclassname);
	        if (tuioclassname.equalsIgnoreCase(classname)){
	            Log.d(TAG, "package found");
	            return true;
	        }
	    }
	    return false;
	 }
}
