/* The following code was written by Matthew Wiggins 
 * and is released under the APACHE 2.0 license 
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 */
package com.sigmusic.tacchi.tuio.preferences;

import com.sigmusic.tacchi.tuio.TuioServiceTools;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Typeface;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class OnOffPreference extends Preference implements OnCheckedChangeListener {
	private static final String androidns = "http://schemas.android.com/apk/res/android";

	
	private static final int TITLE_ID = 1;
	private static final int CHECKBOX_ID = 3;
	private static final int DESCRIPTION_ID = 5;
	
	String summaryOn;
	String summaryOff;
	
//	String titleOn;
//	String titleOff;
	
	TextView description;
	CheckBox checkbox;


	public OnOffPreference(final Context context) {
		super(context);
	}

	public OnOffPreference(final Context context, final AttributeSet attrs) {
		super(context, attrs);
		summaryOn = attrs.getAttributeValue(androidns, "summaryOn");
		summaryOff = attrs.getAttributeValue(androidns, "summaryOff");
//		titleOn = attrs.getAttributeValue(androidns, "titleOn");
//		titleOff = attrs.getAttributeValue(androidns, "titleOff");
	}

	public OnOffPreference(final Context context, final AttributeSet attrs, final int defStyle) {
		super(context, attrs, defStyle);
	}

	@Override
	protected View onCreateView(final ViewGroup parent) {		
		final RelativeLayout layout = new RelativeLayout(getContext());
		layout.setPadding(15, 5, 15, 5);

		
		
		//setup title
		final RelativeLayout.LayoutParams titleparams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.WRAP_CONTENT,
				RelativeLayout.LayoutParams.WRAP_CONTENT);
		titleparams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
		titleparams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);

		final TextView title = new TextView(getContext());
		title.setText(getTitle());
		title.setTextSize(24);
		title.setTextColor(Color.WHITE);
		title.setTypeface(Typeface.DEFAULT, Typeface.NORMAL);
		title.setGravity(Gravity.LEFT);
		title.setLayoutParams(titleparams);
		title.setId(TITLE_ID);
		
		
		//setup description
		final RelativeLayout.LayoutParams descriptionparams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.WRAP_CONTENT,
				RelativeLayout.LayoutParams.WRAP_CONTENT);
		descriptionparams.addRule(RelativeLayout.BELOW, title.getId());
//		descriptionparams.addRule(RelativeLayout.LEFT_OF, CHECKBOX_ID);
		descriptionparams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);

		description = new TextView(getContext());
		description.setText(summaryOff);
		description.setTextSize(16);
		description.setPadding(10, 0, 0, 0);
		description.setTextColor(Color.GRAY);
		description.setTypeface(Typeface.DEFAULT, Typeface.NORMAL);
		description.setGravity(Gravity.LEFT);
		description.setLayoutParams(descriptionparams);
		description.setId(DESCRIPTION_ID);
		
		
		
		//setup slider
		final RelativeLayout.LayoutParams checkboxparams = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.WRAP_CONTENT,
				RelativeLayout.LayoutParams.WRAP_CONTENT);
		checkboxparams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		checkboxparams.addRule(RelativeLayout.CENTER_VERTICAL);

		checkbox = new CheckBox(getContext());
		checkbox.setLayoutParams(checkboxparams);
		checkbox.setOnCheckedChangeListener(this);
		checkbox.setId(CHECKBOX_ID);
		
		

		layout.addView(title);
		layout.addView(description);
		layout.addView(checkbox);
		layout.setId(android.R.id.widget_frame);

		checkService();
		
		return layout;
		
		
	}

	
	
	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
		if (isChecked) {
			turnOnService();
		} else {
			turnOffService();
		}
	}
	
	private void checkService() {
		boolean ison = TuioServiceTools.isServiceRunning(getContext());
		checkbox.setChecked(ison);
		
	}
	
	private void turnOnService() {
    	Intent service = new Intent("com.sigmusic.tacchi.tuio.TUIO_SERVICE");
    	getContext().startService(service);

		description.setText(summaryOn);
	}
	
	private void turnOffService() {
    	Intent service = new Intent("com.sigmusic.tacchi.tuio.TUIO_SERVICE");
    	getContext().stopService(service);

		description.setText(summaryOff);
	}



}
