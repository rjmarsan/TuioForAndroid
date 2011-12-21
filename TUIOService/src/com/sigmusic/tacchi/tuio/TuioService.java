package com.sigmusic.tacchi.tuio;

import java.lang.reflect.Method;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
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

    
    private TuioAndroidClientHoneycomb mClient;
    private Object windowman;
    private WindowManager window;
    private IBinder wmbinder;
    
    private TuioGestureControl gestureControls;
    
    private View overlayView;
    private Handler mHandler;
    
    private Method injectPointerEvent = getInjectPointerEvent();
    Method getInjectPointerEvent() {
    	try {
			return Class.forName("android.view.IWindowManager").getMethod("injectPointerEvent", MotionEvent.class, boolean.class);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
    }
    private Method injectKeyEvent = getInjectKeyEvent();
    Method getInjectKeyEvent() {
    	try {
			return Class.forName("android.view.IWindowManager").getMethod("injectKeyEvent", KeyEvent.class, boolean.class);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
    }
    
    
    @Override
    public void onCreate() {
    	Log.d(TAG, "On create called");
    	
    	/** Open up the preferences and get the prefered port **/
    	SharedPreferences mPrefs = getSharedPreferences(PREFS_NAME, 0);
    	int port = intFromStr(mPrefs, "port", "3333");
    	
    	/** grab the window from the system service that we can grab the width and height from **/
    	window = (WindowManager) getSystemService(Context.WINDOW_SERVICE); 
        Display display = window.getDefaultDisplay();
    	int width = display.getWidth();
    	int height = display.getHeight();
    	Log.d(TAG, "Creating TUIO server with width: "+width+"and height:"+height);
    	
    	/** Start the TUIO client with the port from preferences and display parameters **/
        mClient = new TuioAndroidClientHoneycomb(this,port, width, height);
        
        /** Steal the IWindowManager.  The code from this is stub code as it 
         * relies on stub code found in stub/.  Currently, froyo implements these methods
         * as expected. future versions may not.  **/
        try {
	        wmbinder = (IBinder)Class.forName("android.os.ServiceManager").getMethod("getService", String.class).invoke(null, "window");
	        windowman =  Class.forName("android.view.IWindowManager$Stub").getMethod("asInterface", IBinder.class).invoke(null, wmbinder);//IWindowManager.Stub.asInterface( wmbinder );  
        } catch (Exception e) {
        	e.printStackTrace();
        }
        
        /** this code listens to the raw tuio and detects certain gestures to mimic
         * BACK, HOME, and MENU (more later) **/
        gestureControls = new TuioGestureControl(this, mClient);
        
       
        mHandler = new Handler();
        createOverlay();
    }
    
    /**
     * Puddi puddi.
     */
    private void createOverlay() {
    	overlayView = new View(this);
    	overlayView.setBackgroundColor(Color.TRANSPARENT);
    }
    
    private int intFromStr(SharedPreferences prefs, String key, String defaultVal) {
    	return Integer.parseInt(prefs.getString(key, defaultVal));
    }
    
    public void sendMotionEvent(MotionEvent me) {
    	if (me != null) {
	        Log.i(TAG, "sending motion event");
	        try {
	        	injectPointerEvent.invoke(windowman,me, false);
	        } catch (Exception e) {
	        	e.printStackTrace();
	        }
    	}
    }
    
    public void sendKeyEvent(KeyEvent ke) {
    	if (ke != null) {
	        Log.i(TAG, "sending keypress event");
	        try {
	        	injectKeyEvent.invoke(windowman, ke, false);
	        } catch (Exception e) {
	        	e.printStackTrace();
	        }
    	}
    }
    
    /**
     * Testing the ability for a service to insert views into the current window
     */
    public void insertOverlay() {
    	mHandler.post(new Runnable() { 
    		public void run() {
		    	LayoutParams param = new LayoutParams(LayoutParams.TYPE_SYSTEM_OVERLAY,
						   LayoutParams.FLAG_LAYOUT_IN_SCREEN);
		    	param.token = wmbinder;
		    	window.addView(overlayView, param);
    		}
    	});
    }
    
    public void removeOverlay() {
    	mHandler.post(new Runnable() { 
    		public void run() {
    			try {
	    	    	if (overlayView != null)
	    	    		window.removeView(overlayView);
    			} catch (Exception e) {
    				e.printStackTrace();
    			}
    		}
    	});
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
