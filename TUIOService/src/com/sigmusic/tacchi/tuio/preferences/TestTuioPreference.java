/* The following code was written by Matthew Wiggins 
 * and is released under the APACHE 2.0 license 
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 */
package com.sigmusic.tacchi.tuio.preferences;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.sigmusic.tacchi.tuio.TuioTest;

public class TestTuioPreference extends Preference implements OnClickListener {
	private static final String androidns = "http://schemas.android.com/apk/res/android";

	
	private static final int TITLE_ID = 1;
	private static final int CHECKBOX_ID = 3;
	private static final int DESCRIPTION_ID = 5;
	

	public TestTuioPreference(final Context context) {
		super(context);
	}

	public TestTuioPreference(final Context context, final AttributeSet attrs) {
		super(context, attrs);
	}

	public TestTuioPreference(final Context context, final AttributeSet attrs, final int defStyle) {
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

		layout.addView(title);
		layout.setOnClickListener(this);
		layout.setId(android.R.id.widget_frame);

		return layout;
		
		
	}

	@Override
	public void onClick(View v) {
		Intent i = new Intent(getContext(), TuioTest.class);
		getContext().startActivity(i);
	}



	
	



}
