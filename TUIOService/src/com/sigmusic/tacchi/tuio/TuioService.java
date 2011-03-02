package com.sigmusic.tacchi.tuio;

import android.app.Instrumentation;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.ServiceManager;
import android.util.Log;
import android.view.IWindowManager;
import android.view.MotionEvent;
import android.widget.Toast;

public class TuioService extends Service {
	public final static String TAG = "TuioService";

	
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
    private Instrumentation inst;
    private IWindowManager windowman;
    
    
    @Override
    public void onCreate() {
    	Log.d(TAG, "On create called");
        mClient = new TuioAndroidClient(this, 800, 600);
        inst = new Instrumentation();
        IBinder wmbinder = ServiceManager.getService( "window" );
        Log.d( TAG, "WindowManager: "+wmbinder );
        windowman =  IWindowManager.Stub.asInterface( wmbinder );

        
    }
    
    public void sendMotionEvent(MotionEvent me) {
        Log.i(TAG, "sending motion event");
//    	inst.sendPointerSync(me);
    	windowman.injectPointerEvent(me, false);
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
