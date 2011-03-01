package com.sigmusic.tacchi.tuio;

import java.util.HashMap;

import TUIO.TuioClient;
import TUIO.TuioCursor;
import TUIO.TuioListener;
import TUIO.TuioObject;
import TUIO.TuioTime;
import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;

public class TuioAndroidClient implements TuioListener {
	public final static String TAG = "TuioAndroidClient";
	TuioClient client;
	TuioService callback;
	int width, height;
	
	HashMap<Integer, MotionEvent> motionEventMap = new HashMap<Integer, MotionEvent>();
	
	public TuioAndroidClient(TuioService callback, int width, int height) {
		client = new TuioClient();
		client.addTuioListener(this);
		this.callback = callback;
		this.width = width;
		this.height = height;
	}
	
	public void start() {
		Log.v(TAG, "Starting TUIO client on port:" +3333);
		client.connect();
	}
	
	public void stop() {
		Log.v(TAG, "Stopping client");
		client.disconnect();
	}
	
	
	

	@Override
	public void addTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor down: "+tcur.toString());
		MotionEvent me = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, tcur.getScreenX(width), tcur.getScreenY(height), 0);
//		motionEventMap.put(tcur.getCursorID(), me);
		callback.sendMotionEvent(me);
	}

	@Override
	public void addTuioObject(TuioObject tobj) {
	}
	

	@Override
	public void updateTuioCursor(TuioCursor tcur) {
//		int key = tcur.getCursorID();
//		if (motionEventMap.containsKey(key)) {
//			MotionEvent me = motionEventMap.get(me);
//			if (me != null) {
//				me.
//			}
//		}
		Log.d(TAG, "Cursor moved: "+tcur.toString());
		MotionEvent me = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_MOVE, tcur.getScreenX(width), tcur.getScreenY(height), 0);
		callback.sendMotionEvent(me);
	}

	@Override
	public void updateTuioObject(TuioObject tobj) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void removeTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor up: "+tcur.toString());
		MotionEvent me = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_UP, tcur.getScreenX(width), tcur.getScreenY(height), 0);
		callback.sendMotionEvent(me);
	}

	@Override
	public void removeTuioObject(TuioObject tobj) {
		// TODO Auto-generated method stub
		
	}


	@Override
	public void refresh(TuioTime ftime) {		
	}


}
