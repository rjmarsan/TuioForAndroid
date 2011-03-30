package com.sigmusic.tacchi.tuio;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.os.ServiceManager;
import android.util.Log;
import android.view.Display;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.widget.Toast;

public class TuioService extends Service {
	public final static String TAG = "TuioService";
	public final static String PREFS_NAME = "tuioserviceprefs";

	
    /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        TuioService getService() {
            return TuioService.this;
        }
    }

    
    private TuioAndroidClient mClient;
    private IWindowManager windowman;
//    private WindowManager window;
    
    private TuioGestureControl gestureControls;
    
    @Override
    public void onCreate() {
    	Log.d(TAG, "On create called");
    	SharedPreferences mPrefs = getSharedPreferences(PREFS_NAME, 0);
    	int port = intFromStr(mPrefs, "port", "3333");
        Display display = ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
    	int width = display.getWidth();
    	int height = display.getHeight();
    	Log.d(TAG, "Creating TUIO server with width: "+width+"and height:"+height);
        mClient = new TuioAndroidClient(this,port, width, height);
        IBinder wmbinder = ServiceManager.getService( "window" );
//        Log.d( TAG, "WindowManager: "+wmbinder );
        windowman =  IWindowManager.Stub.asInterface( wmbinder );  
        //gestureControls = new TuioGestureControl(this, mClient);
    }
    
    private int intFromStr(SharedPreferences prefs, String key, String defaultVal) {
    	return Integer.parseInt(prefs.getString(key, defaultVal));
    }
    
    public void sendMotionEvent(MotionEvent me) {
    	if (me != null) {
	        Log.i(TAG, "sending motion event");
	    	windowman.injectPointerEvent(me, false);
    	}
    }
    
    public void sendKeyEvent(KeyEvent ke) {
    	if (ke != null) {
	        Log.i(TAG, "sending keypress event");
	    	windowman.injectKeyEvent(ke, false);
    	}
    }
    

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        Log.i(TAG, "starting client");
        mClient.start();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
    	mClient.stop();
        // Tell the user we stopped.
        Toast.makeText(this, "Local service stopped", Toast.LENGTH_SHORT).show();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();

}
