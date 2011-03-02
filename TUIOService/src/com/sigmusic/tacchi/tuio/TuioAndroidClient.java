package com.sigmusic.tacchi.tuio;

import java.util.HashMap;

import TUIO.TuioClient;
import TUIO.TuioContainer;
import TUIO.TuioCursor;
import TUIO.TuioListener;
import TUIO.TuioObject;
import TUIO.TuioPoint;
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
	
	
	private MotionEvent makeMotionEvent(TuioContainer point, int id, int action) {
		long startms = point.getStartTime().getTotalMilliseconds();
		long totalms = point.getTuioTime().getTotalMilliseconds();
		int totalcursors = client.getTuioCursors().size()+client.getTuioObjects().size();
		int actionmasked = action |( 0x1 << (7+id) & 0xff00);
		
		
		Log.d("TuioEvent", "TuioContainer: "+point.toString()+" startms: "+startms+" totalms: "+totalms+" cursors: "+totalcursors+" id: "+id+ " action: "+action+" Action masked: "+actionmasked);
		
		MotionEvent me = MotionEvent.obtain(startms, totalms, actionmasked, totalcursors, point.getScreenX(width), point.getScreenY(height), 1, 0.1f, 0, 0, 0, 0, 0);
		return me;
	}
	
	

	@Override
	public void addTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor down: "+tcur.toString());
		addTuioThing(tcur, tcur.getCursorID());
	}

	@Override
	public void addTuioObject(TuioObject tobj) {
		Log.d(TAG, "Fiducial down: "+tobj.toString());
		addTuioThing(tobj, tobj.getSymbolID());
	}
	
	
	private void addTuioThing(TuioContainer point, int id) {
//		Log.d(TAG, "forwarding");
		int event = (id == 0) ? MotionEvent.ACTION_DOWN : MotionEvent.ACTION_POINTER_DOWN;
		MotionEvent me = makeMotionEvent(point, id, event);
		motionEventMap.put(id, me);
		callback.sendMotionEvent(me);
	}
	

	@Override
	public void updateTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor moved: "+tcur.toString());
		updateTuioThing(tcur, tcur.getCursorID());
	}

	@Override
	public void updateTuioObject(TuioObject tobj) {
		Log.d(TAG, "fiducial moved: "+tobj.toString());
		updateTuioThing(tobj, tobj.getSymbolID());
	}
	
	private void updateTuioThing(TuioContainer point, int id) {
		int key = id;
		if (motionEventMap.containsKey(key)) {
			MotionEvent me = motionEventMap.get(key);
			if (me != null) {
				long totalms = point.getTuioTime().getTotalMilliseconds();
				me = MotionEvent.obtain(me);
				me.addBatch(totalms, point.getScreenX(width), point.getScreenY(height), 1, 0.1f, 0);
				me.setAction(MotionEvent.ACTION_MOVE);
				//				Log.d(TAG, "forwarding");
				motionEventMap.put(key, me);
				callback.sendMotionEvent(me);
			}
		}

	}

	@Override
	public void removeTuioCursor(TuioCursor tcur) {
		Log.d(TAG, "Cursor up: "+tcur.toString());
		removeTuioThing(tcur, tcur.getCursorID());
	}

	@Override
	public void removeTuioObject(TuioObject tobj) {
		Log.d(TAG, "Fiducal up: "+tobj.toString());
		removeTuioThing(tobj, tobj.getSymbolID());
	}
	
	private void removeTuioThing(TuioContainer point, int id) {
//		Log.d(TAG, "forwarding");
		int event = (id == 0) ? MotionEvent.ACTION_UP : MotionEvent.ACTION_POINTER_UP;
		MotionEvent me = makeMotionEvent(point, id, event);
		motionEventMap.remove(id);
		callback.sendMotionEvent(me);
	}


	@Override
	public void refresh(TuioTime ftime) {		
	}


}
