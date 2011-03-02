package com.sigmusic.tacchi.tuio;

import TUIO.TuioClient;
import TUIO.TuioContainer;
import TUIO.TuioCursor;
import TUIO.TuioListener;
import TUIO.TuioObject;
import TUIO.TuioTime;
import android.util.Log;
import android.view.MotionEvent;

public class TuioAndroidClient implements TuioListener {
	public final static String TAG = "TuioAndroidClient";
	TuioClient client;
	TuioService callback;
	int width, height;
	
	MotionEvent currentEvent;
	
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
	
	
	
	private int getNumCursors() {
		return client.getTuioCursors().size() + client.getTuioObjects().size();
	}
	private MotionEvent makeOrUpdateMotionEvent(TuioContainer point, int id, int action) {
		long startms = point.getStartTime().getTotalMilliseconds();
		long totalms = point.getTuioTime().getTotalMilliseconds();
		int totalcursors = getNumCursors();
		int actionmasked = action |( 0x1 << (7+id) & 0xff00);
		
		Log.d("TuioEvent", "TuioContainer: "+point.toString()+" startms: "+startms+" totalms: "+totalms+" cursors: "+totalcursors+" id: "+id+ " action: "+action+" Action masked: "+actionmasked);
		
	    /**	     * 
	     * @param eventTimeNano  The the time (in ns) when this specific event was generated.  This 
	     * must be obtained from {@link System#nanoTime()}.
	     * @param action The kind of action being performed -- one of either
	     * {@link #ACTION_DOWN}, {@link #ACTION_MOVE}, {@link #ACTION_UP}, or
	     * {@link #ACTION_CANCEL}.
	     * @param pointers The number of points that will be in this event.
	     * @param inPointerIds An array of <em>pointers</em> values providing
	     * an identifier for each pointer.
	     * @param inData An array of <em>pointers*NUM_SAMPLE_DATA</em> of initial
	     * data samples for the event.
	     * @param metaState The state of any meta / modifier keys that were in effect when
	     * the event was generated.
	     * @param xPrecision The precision of the X coordinate being reported.
	     * @param yPrecision The precision of the Y coordinate being reported.
	     * @param deviceId The id for the device that this event came from.  An id of
	     * zero indicates that the event didn't come from a physical device; other
	     * numbers are arbitrary and you shouldn't depend on the values.
	     * @param edgeFlags A bitfield indicating which edges, if any, where touched by this
	     * MotionEvent.
	     *
	     * @hide
	     */
		
		int[] pointerIds = new int[totalcursors];
		int i =0;
		float[] inData = new float[totalcursors * 4]; // 4 = MotionEvent.NUM_SAMPLE_DATA;
		for (TuioObject obj : client.getTuioObjects()) {
			pointerIds[i] = obj.getSymbolID();
			
			inData[i*4] = obj.getScreenX(width);
			inData[i*4+1] = obj.getScreenY(height);
			inData[i*4+2] = 1;//pressure
			inData[i*4+3] = 0.1f; //size
			i++;

		}
		for (TuioCursor obj : client.getTuioCursors()) {
			pointerIds[i] = obj.getCursorID();
			inData[i*4] = obj.getScreenX(width);
			inData[i*4+1] = obj.getScreenY(height);
			inData[i*4+2] = 1;//pressure
			inData[i*4+3] = 0.1f; //size
			i++;

		}
		

		if (action != MotionEvent.ACTION_MOVE)
			currentEvent = MotionEvent.obtainNano(startms, totalms, System.nanoTime(), actionmasked, totalcursors, pointerIds, inData, 0, 0, 0, 0, 0);
		else 
			currentEvent.addBatch(totalms, inData, 0);
		
		
		
		
		
		return currentEvent;
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
		MotionEvent me = makeOrUpdateMotionEvent(point, id, event);
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

		int event = MotionEvent.ACTION_MOVE;
		MotionEvent me = makeOrUpdateMotionEvent(point, id, event);
		callback.sendMotionEvent(me);

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
		MotionEvent me = makeOrUpdateMotionEvent(point, id, event);
		callback.sendMotionEvent(me);
	}


	@Override
	public void refresh(TuioTime ftime) {		
	}


}
