//Copyright+LGPL
#include <vcl.h>
#pragma hdrstop

#include "Main.h"
#include "mininec3.h"
#include "AntView.h"
#include "AntEditor.h"

//---------------------------------------------------------------------------
static void FormatAntLengthText(char *bf, LPCSTR Label, double Len)
{
	sprintf(bf, "%s:%s %s", Label, StrDbl(GetRmdVal(Len)), GetLenUnitText());
}
//---------------------------------------------------------------------------
static void PaintAntLengthLabel(TCanvas *Canvas, int Width, int Height, int X, int Y, LPCSTR Text)
{
	int tw = Canvas->TextWidth(Text);
	int th = Canvas->TextHeight(Text);
	int tx = X + 12;
	int ty = Y + 12;
	if( tx + tw + 8 > Width ) tx = X - tw - 12;
	if( ty + th + 6 > Height ) ty = Y - th - 12;
	if( tx < 0 ) tx = 0;
	if( ty < 0 ) ty = 0;

	TColor oldPen = Canvas->Pen->Color;
	TColor oldBrush = Canvas->Brush->Color;
	TColor oldFont = Canvas->Font->Color;
	TPenStyle oldPenStyle = Canvas->Pen->Style;
	TBrushStyle oldBrushStyle = Canvas->Brush->Style;
	TRect rc;
	rc.Left = tx - 3;
	rc.Top = ty - 2;
	rc.Right = tx + tw + 5;
	rc.Bottom = ty + th + 4;
	Canvas->Pen->Color = clBlack;
	Canvas->Pen->Style = psSolid;
	Canvas->Brush->Color = clInfoBk;
	Canvas->Brush->Style = bsSolid;
	Canvas->Rectangle(rc.Left, rc.Top, rc.Right, rc.Bottom);
	Canvas->Font->Color = clBlack;
	Canvas->TextOut(tx, ty, Text);
	Canvas->Pen->Color = oldPen;
	Canvas->Brush->Color = oldBrush;
	Canvas->Font->Color = oldFont;
	Canvas->Pen->Style = oldPenStyle;
	Canvas->Brush->Style = oldBrushStyle;
}

//---------------------------------------------------------------------------
static TColor AntGizmoAxisColor(int Axis, int Muted)
{
	if( Muted ){
		switch( Axis ){
			case ANT_GIZMO_AXIS_X: return (TColor)RGB(255, 150, 150);
			case ANT_GIZMO_AXIS_Y: return (TColor)RGB(150, 205, 150);
			case ANT_GIZMO_AXIS_Z: return (TColor)RGB(150, 170, 255);
		}
	}
	switch( Axis ){
		case ANT_GIZMO_AXIS_X: return clRed;
		case ANT_GIZMO_AXIS_Y: return clGreen;
		case ANT_GIZMO_AXIS_Z: return clBlue;
	}
	return clBlack;
}

static LPCSTR AntGizmoAxisText(int Axis)
{
	switch( Axis ){
		case ANT_GIZMO_AXIS_X: return "X";
		case ANT_GIZMO_AXIS_Y: return "Y";
		case ANT_GIZMO_AXIS_Z: return "Z";
	}
	return "";
}

static TEdit *AntGizmoCreateEdit(TForm *Dlg, int Left, int Top, int Width, int Enabled)
{
	TEdit *Edit = new TEdit(Dlg);
	Edit->Parent = Dlg;
	Edit->Left = Left;
	Edit->Top = Top;
	Edit->Width = Width;
	Edit->Text = "0";
	Edit->Enabled = Enabled;
	return Edit;
}

static TLabel *AntGizmoCreateLabel(TForm *Dlg, LPCSTR Text, int Left, int Top)
{
	TLabel *Label = new TLabel(Dlg);
	Label->Parent = Dlg;
	Label->Left = Left;
	Label->Top = Top + 4;
	Label->Caption = Text;
	return Label;
}

static int AntGizmoCalcEdit(TEdit *Edit, double &Value)
{
	AnsiString Text(Edit->Text);
	if( Text.Length() <= 0 ) Text = "0";
	return Calc(Value, Text.c_str());
}

static int AntGizmoPromptInput(TComponent *Owner, int Mode, int Axis,
	double &DX, double &DY, double &DZ, double &Angle)
{
	DX = DY = DZ = Angle = 0.0;

	TForm *Dlg = new TForm(Owner);
	Dlg->BorderStyle = bsDialog;
	Dlg->Position = poScreenCenter;
	Dlg->ClientWidth = 270;
	Dlg->Caption = (Mode == ANT_GIZMO_MODE_ROTATE) ? "Gizmo Rotate" : "Gizmo Move";

	TEdit *EditX = NULL;
	TEdit *EditY = NULL;
	TEdit *EditZ = NULL;
	TEdit *EditDeg = NULL;

	if( Mode == ANT_GIZMO_MODE_ROTATE ){
		Dlg->ClientHeight = 112;
		AnsiString AxisText = "Axis: ";
		AxisText += AntGizmoAxisText(Axis);
		AntGizmoCreateLabel(Dlg, AxisText.c_str(), 14, 12);
		AntGizmoCreateLabel(Dlg, "Angle (deg)", 14, 42);
		EditDeg = AntGizmoCreateEdit(Dlg, 104, 38, 138, TRUE);
		Dlg->ActiveControl = EditDeg;
	}
	else {
		Dlg->ClientHeight = 162;
		AnsiString Info = "Delta (";
		Info += GetLenUnitText();
		Info += ")";
		AntGizmoCreateLabel(Dlg, Info.c_str(), 14, 12);
		AntGizmoCreateLabel(Dlg, "X", 32, 42);
		AntGizmoCreateLabel(Dlg, "Y", 32, 68);
		AntGizmoCreateLabel(Dlg, "Z", 32, 94);
		EditX = AntGizmoCreateEdit(Dlg, 72, 38, 170, (Axis < 0) || (Axis == ANT_GIZMO_AXIS_X));
		EditY = AntGizmoCreateEdit(Dlg, 72, 64, 170, (Axis < 0) || (Axis == ANT_GIZMO_AXIS_Y));
		EditZ = AntGizmoCreateEdit(Dlg, 72, 90, 170, (Axis < 0) || (Axis == ANT_GIZMO_AXIS_Z));
		if( EditX->Enabled ) Dlg->ActiveControl = EditX;
		else if( EditY->Enabled ) Dlg->ActiveControl = EditY;
		else Dlg->ActiveControl = EditZ;
	}

	TButton *OKBtn = new TButton(Dlg);
	OKBtn->Parent = Dlg;
	OKBtn->Caption = "OK";
	OKBtn->Left = 78;
	OKBtn->Top = Dlg->ClientHeight - 34;
	OKBtn->Width = 80;
	OKBtn->ModalResult = mrOk;
	OKBtn->Default = true;

	TButton *CancelBtn = new TButton(Dlg);
	CancelBtn->Parent = Dlg;
	CancelBtn->Caption = "Cancel";
	CancelBtn->Left = 166;
	CancelBtn->Top = Dlg->ClientHeight - 34;
	CancelBtn->Width = 80;
	CancelBtn->ModalResult = mrCancel;
	CancelBtn->Cancel = true;

	int ModalResult = Dlg->ShowModal();
	int Result = FALSE;
	if( ModalResult == mrOk ){
		if( Mode == ANT_GIZMO_MODE_ROTATE ){
			Result = AntGizmoCalcEdit(EditDeg, Angle);
		}
		else {
			Result = AntGizmoCalcEdit(EditX, DX) &&
				AntGizmoCalcEdit(EditY, DY) &&
				AntGizmoCalcEdit(EditZ, DZ);
			if( Result == TRUE ){
				DX = SetRmdVal(DX);
				DY = SetRmdVal(DY);
				DZ = SetRmdVal(DZ);
			}
		}
	}
	delete Dlg;
	if( (ModalResult == mrOk) && (Result != TRUE) ) ::MessageBeep(MB_ICONEXCLAMATION);
	return (ModalResult == mrOk) && (Result == TRUE);
}

static void AntGizmoRingPoint(int Axis, double CX, double CY, double CZ,
	double Radius, double Angle, double &X, double &Y, double &Z)
{
	double c = cos(Angle);
	double s = sin(Angle);
	X = CX;
	Y = CY;
	Z = CZ;
	switch( Axis ){
		case ANT_GIZMO_AXIS_X:
			Y += Radius * c;
			Z += Radius * s;
			break;
		case ANT_GIZMO_AXIS_Y:
			X += Radius * c;
			Z += Radius * s;
			break;
		case ANT_GIZMO_AXIS_Z:
			X += Radius * c;
			Y += Radius * s;
			break;
	}
}

static int AntGizmoRotationPlane(int Axis)
{
	switch( Axis ){
		case ANT_GIZMO_AXIS_X: return ANT_DRAW_PLANE_YZ;
		case ANT_GIZMO_AXIS_Y: return ANT_DRAW_PLANE_XZ;
		case ANT_GIZMO_AXIS_Z: return ANT_DRAW_PLANE_XY;
	}
	return ANT_DRAW_PLANE_XY;
}

static double AntGizmoWorldRotationAngle(int Axis, double CX, double CY, double CZ,
	double X, double Y, double Z)
{
	switch( Axis ){
		case ANT_GIZMO_AXIS_X: return atan2(Z - CZ, Y - CY) * 180.0 / PAI;
		case ANT_GIZMO_AXIS_Y: return atan2(Z - CZ, X - CX) * 180.0 / PAI;
		case ANT_GIZMO_AXIS_Z: return atan2(Y - CY, X - CX) * 180.0 / PAI;
	}
	return 0.0;
}

static double AntGizmoNormalizeAngle(double Angle)
{
	while( Angle > 180.0 ) Angle -= 360.0;
	while( Angle < -180.0 ) Angle += 360.0;
	return Angle;
}

static double AntGizmoSnapAngle(double Angle, double Step)
{
	if( Step <= 0.0 ) return Angle;
	double n = Angle / Step;
	int rounded = (n >= 0.0) ? int(n + 0.5) : int(n - 0.5);
	return double(rounded) * Step;
}

static void AntRotatePointAroundGizmoAxis(double &X, double &Y, double &Z,
	double CX, double CY, double CZ, int Axis, double Angle)
{
	double x = X - CX;
	double y = Y - CY;
	double z = Z - CZ;
	double r = Angle * PAI / 180.0;
	double c = cos(r);
	double s = sin(r);
	switch( Axis ){
		case ANT_GIZMO_AXIS_X:
			Y = CY + (y * c) - (z * s);
			Z = CZ + (y * s) + (z * c);
			break;
		case ANT_GIZMO_AXIS_Y:
			X = CX + (x * c) - (z * s);
			Z = CZ + (x * s) + (z * c);
			break;
		case ANT_GIZMO_AXIS_Z:
			X = CX + (x * c) - (y * s);
			Y = CY + (x * s) + (y * c);
			break;
	}
}

static void AntRotateWireAroundGizmoAxis(WDEF *Wire, double CX, double CY, double CZ,
	int Axis, double Angle)
{
	AntRotatePointAroundGizmoAxis(Wire->X1, Wire->Y1, Wire->Z1, CX, CY, CZ, Axis, Angle);
	AntRotatePointAroundGizmoAxis(Wire->X2, Wire->Y2, Wire->Z2, CX, CY, CZ, Axis, Angle);
}

static int AntRectContainsPoint(int L, int T, int R, int B, int X, int Y)
{
	return (X >= L) && (X <= R) && (Y >= T) && (Y <= B);
}

static double AntSegmentCross(int AX, int AY, int BX, int BY, int CX, int CY)
{
	return double(BX - AX) * double(CY - AY) - double(BY - AY) * double(CX - AX);
}

static int AntValueBetween(int A, int B, int C)
{
	return ((A <= C) && (C <= B)) || ((B <= C) && (C <= A));
}

static int AntPointOnSegment(int AX, int AY, int BX, int BY, int PX, int PY)
{
	if( AntSegmentCross(AX, AY, BX, BY, PX, PY) != 0.0 ) return FALSE;
	return AntValueBetween(AX, BX, PX) && AntValueBetween(AY, BY, PY);
}

static int AntSegmentsIntersect(int A1X, int A1Y, int A2X, int A2Y,
	int B1X, int B1Y, int B2X, int B2Y)
{
	double c1 = AntSegmentCross(A1X, A1Y, A2X, A2Y, B1X, B1Y);
	double c2 = AntSegmentCross(A1X, A1Y, A2X, A2Y, B2X, B2Y);
	double c3 = AntSegmentCross(B1X, B1Y, B2X, B2Y, A1X, A1Y);
	double c4 = AntSegmentCross(B1X, B1Y, B2X, B2Y, A2X, A2Y);

	if( (c1 == 0.0) && AntPointOnSegment(A1X, A1Y, A2X, A2Y, B1X, B1Y) ) return TRUE;
	if( (c2 == 0.0) && AntPointOnSegment(A1X, A1Y, A2X, A2Y, B2X, B2Y) ) return TRUE;
	if( (c3 == 0.0) && AntPointOnSegment(B1X, B1Y, B2X, B2Y, A1X, A1Y) ) return TRUE;
	if( (c4 == 0.0) && AntPointOnSegment(B1X, B1Y, B2X, B2Y, A2X, A2Y) ) return TRUE;

	return (((c1 < 0) && (c2 > 0)) || ((c1 > 0) && (c2 < 0))) &&
		(((c3 < 0) && (c4 > 0)) || ((c3 > 0) && (c4 < 0)));
}

static int AntSegmentIntersectsRect(int X1, int Y1, int X2, int Y2,
	int L, int T, int R, int B)
{
	if( AntRectContainsPoint(L, T, R, B, X1, Y1) ) return TRUE;
	if( AntRectContainsPoint(L, T, R, B, X2, Y2) ) return TRUE;
	if( ((X1 < L) && (X2 < L)) || ((X1 > R) && (X2 > R)) ||
		((Y1 < T) && (Y2 < T)) || ((Y1 > B) && (Y2 > B)) ) return FALSE;
	if( AntSegmentsIntersect(X1, Y1, X2, Y2, L, T, R, T) ) return TRUE;
	if( AntSegmentsIntersect(X1, Y1, X2, Y2, R, T, R, B) ) return TRUE;
	if( AntSegmentsIntersect(X1, Y1, X2, Y2, R, B, L, B) ) return TRUE;
	if( AntSegmentsIntersect(X1, Y1, X2, Y2, L, B, L, T) ) return TRUE;
	return FALSE;
}

static int AntSamePoint(double X1, double Y1, double Z1, double X2, double Y2, double Z2)
{
	return (X1 == X2) && (Y1 == Y2) && (Z1 == Z2);
}

static void AntMoveConnectedPoint(WDEF *Wires, const WDEF *OldWires, int WMax,
	double OldX, double OldY, double OldZ, double NewX, double NewY, double NewZ)
{
	if( AntSamePoint(OldX, OldY, OldZ, NewX, NewY, NewZ) ) return;
	for( int i = 0; i < WMax; i++ ){
		if( AntSamePoint(OldWires[i].X1, OldWires[i].Y1, OldWires[i].Z1, OldX, OldY, OldZ) ){
			Wires[i].X1 = NewX;
			Wires[i].Y1 = NewY;
			Wires[i].Z1 = NewZ;
		}
		if( AntSamePoint(OldWires[i].X2, OldWires[i].Y2, OldWires[i].Z2, OldX, OldY, OldZ) ){
			Wires[i].X2 = NewX;
			Wires[i].Y2 = NewY;
			Wires[i].Z2 = NewZ;
		}
	}
}

static void AntMoveConnectedWirePoints(WDEF *Wires, const WDEF *OldWires, int WMax,
	const WDEF &OldWire, const WDEF &NewWire)
{
	AntMoveConnectedPoint(Wires, OldWires, WMax,
		OldWire.X1, OldWire.Y1, OldWire.Z1, NewWire.X1, NewWire.Y1, NewWire.Z1);
	AntMoveConnectedPoint(Wires, OldWires, WMax,
		OldWire.X2, OldWire.Y2, OldWire.Z2, NewWire.X2, NewWire.Y2, NewWire.Z2);
}

static int AntCtrlDown(TShiftState Shift)
{
	return Shift.Contains(ssCtrl) || ((::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
}

static int AntShiftDown(TShiftState Shift)
{
	return Shift.Contains(ssShift) || ((::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
}

static TCanvas *AntPaintBufferCanvas = NULL;

//---------------------------------------------------------------------------
TCanvas *__fastcall TMainWnd::AntPaintCanvas(void)
{
	return (AntPaintBufferCanvas != NULL) ? AntPaintBufferCanvas : PBoxAnt->Canvas;
}

//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntViewParentWindowProc(TMessage &Message, TWndMethod OldWindowProc, TWinControl *ParentControl)
{
	if( (Message.Msg == WM_ERASEBKGND) && (PBoxAnt != NULL) &&
		(PBoxAnt->Parent == ParentControl) && PBoxAnt->Visible ){
		HDC dc = (HDC)Message.WParam;
		if( dc != NULL ){
			int save = ::SaveDC(dc);
			if( save ){
				::ExcludeClipRect(dc, PBoxAnt->Left, PBoxAnt->Top,
					PBoxAnt->Left + PBoxAnt->Width, PBoxAnt->Top + PBoxAnt->Height);
				OldWindowProc(Message);
				::RestoreDC(dc, save);
				return;
			}
		}
	}
	OldWindowProc(Message);
}

//---------------------------------------------------------------------------
void __fastcall TMainWnd::TabSheet2WindowProc(TMessage &Message)
{
	AntViewParentWindowProc(Message, OldTabSheet2WindowProc, TabSheet2);
}

//---------------------------------------------------------------------------
void __fastcall TMainWnd::PanelTopRightWindowProc(TMessage &Message)
{
	AntViewParentWindowProc(Message, OldPanelTopRightWindowProc, PanelTopRight);
}

//---------------------------------------------------------------------------
// アンテナ形状の表示イベント
void __fastcall TMainWnd::PBoxAntPaint(TObject *Sender)
{
	int AntPaintWidth = PBoxAnt->Width;
	int AntPaintHeight = PBoxAnt->Height;
	if( (AntPaintWidth <= 0) || (AntPaintHeight <= 0) ) return;

	TBitmap *AntPaintBitmap = new TBitmap;
	TCanvas *OldAntPaintCanvas = AntPaintBufferCanvas;
	try {
		AntPaintBitmap->PixelFormat = pf24bit;
		AntPaintBitmap->Width = AntPaintWidth;
		AntPaintBitmap->Height = AntPaintHeight;
		AntPaintBitmap->Canvas->Font->Assign(this->PBoxAnt->Canvas->Font);
		AntPaintBitmap->Canvas->Pen->Assign(this->PBoxAnt->Canvas->Pen);
		AntPaintBitmap->Canvas->Brush->Assign(this->PBoxAnt->Canvas->Brush);
		AntPaintBufferCanvas = AntPaintBitmap->Canvas;

		struct TAntPaintBoxProxy {
			TCanvas *Canvas;
			int Width;
			int Height;
		};
		TAntPaintBoxProxy AntPaintBox = { AntPaintBitmap->Canvas, AntPaintWidth, AntPaintHeight };
		TAntPaintBoxProxy *PBoxAnt = &AntPaintBox;

	TRect	rc;

	rc.Left = 0;
	rc.Top = 0;
	rc.Right = PBoxAnt->Width;
	rc.Bottom = PBoxAnt->Height;
	PBoxAnt->Canvas->Brush->Color = clWhite;
	PBoxAnt->Canvas->FillRect(rc);
	Clip.SetClip(-100, -100, rc.Right + 100, rc.Bottom + 100);
	double sc = double(TBarSC->Position) / 20.0;
	sc = sc * sc * sc * sc;
	int		Xc = int(PBoxAnt->Width/2 + (exeenv.AntXc * sc));
	int		Yc = int(PBoxAnt->Height/2 + (exeenv.AntYc * sc));

	PBoxAnt->Canvas->Pen->Color = clWhite;
	double x, y;
	int		X,Y, X2, Y2;
	double deg = double(TBarDeg->Position);
	deg *= (PAI / 180.0);
	double zdeg = double(TBarZDeg->Position);
	zdeg *= (PAI / 180.0);

	PBoxAnt->Canvas->Pen->Color = clLtGray;
	Calc3DXY(x, y, deg, zdeg, 0, 0, 0);				// Y
	X = int(x) + Xc;
	Y = Yc - int(y);
	PBoxAnt->Canvas->MoveTo(X, Y);
	Calc3DXY(x, y, deg, zdeg, 0, PBoxAnt->Width/2-16, 0);
	X = int(x) + Xc;
	Y = Yc - int(y);
	PBoxAnt->Canvas->LineTo(X, Y);
	int Sop = ::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
	PBoxAnt->Canvas->TextOut(X, Y, "Y");

	Calc3DXY(x, y, deg, zdeg, 0, 0, 0);				// Z
	X = int(x) + Xc;
	Y = Yc - int(y);
	PBoxAnt->Canvas->MoveTo(X, Y);
	Calc3DXY(x, y, deg, zdeg, 0, 0, PBoxAnt->Height);
	X = int(x) + Xc;
	Y = Yc - int(y);
	PBoxAnt->Canvas->LineTo(X, Y);
	::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
	PBoxAnt->Canvas->TextOut(X+4, 0, "Z");

	Calc3DXY(x, y, deg, zdeg, 0, 0, 0);				// X
	X = int(x) + Xc;
	Y = Yc - int(y);
	PBoxAnt->Canvas->MoveTo(X, Y);
	Calc3DXY(x, y, deg, zdeg, PBoxAnt->Height/2, 0, 0);
	X = int(x) + Xc;
	Y = Yc - int(y);
	PBoxAnt->Canvas->LineTo(X, Y);
	::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
	PBoxAnt->Canvas->TextOut(X, Y, "X");

	// ワイヤの表示
	PBoxAnt->Canvas->Pen->Color = clBlack;
	int i;
	WDEF *wp = pCalAnt->wdef;
	for( i = 0; i < pCalAnt->wmax; i++, wp++ ){
		PBoxAnt->Canvas->Pen->Color = wp->R ? clBlack : clGray;
		int sw = ant.wmax ? (i % ant.wmax) : i;
		PBoxAnt->Canvas->Pen->Width = IsAntWireSelected(sw) ? 2 : 1;
		CalcAntViewXY(x, y, deg, zdeg, wp->X1, wp->Y1, wp->Z1);
		X = int((x * sc)) + Xc;
		Y = Yc - int((y * sc));
		CalcAntViewXY(x, y, deg, zdeg, wp->X2, wp->Y2, wp->Z2);
		X2 = int((x * sc)) + Xc;
		Y2 = Yc - int((y * sc));
		if( Clip.Clip(X, Y, X2, Y2) == TRUE ){
			PBoxAnt->Canvas->MoveTo(X, Y);
			PBoxAnt->Canvas->LineTo(X2, Y2);
		}
		if( wp->R < 0.0 ){	// 組み合わせワイヤ
			PDEF *pp = FindPP(pCalAnt, wp->R);
			if( pp != NULL ){
				PBoxAnt->Canvas->Pen->Width = 1;
				PBoxAnt->Canvas->Pen->Color = clBlue;
				WDEF	temp[PPMAX*2+2];
				int n = CombWire(temp, pCalAnt, pp, wp) - 1;
				int j;
				WDEF *tp;
				for( tp = temp, j = 0; j < n; j++, tp++ ){
					CalcAntViewXY(x, y, deg, zdeg, tp->X2, tp->Y2, tp->Z2);
					X = int((x * sc)) + Xc;
					Y = Yc - int((y * sc));
					::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
					PBoxAnt->Canvas->MoveTo(X-2, Y-2);
					PBoxAnt->Canvas->LineTo(X+2, Y-2); PBoxAnt->Canvas->LineTo(X+2, Y+2);
					PBoxAnt->Canvas->LineTo(X-2, Y+2); PBoxAnt->Canvas->LineTo(X-2, Y-2);
				}
				PBoxAnt->Canvas->Pen->Color = clBlack;
			}
		}
	}
	PBoxAnt->Canvas->Pen->Width = 1;
	PaintAntEditGizmo();
	PaintAntSnapVertices();
	PaintAntDrawPreview();
	PaintAntDrawSnapPoint();
	double	cx, cy, cz;
	if( pCalAnt->wmax && DspPlus->Checked ){		// セグメント分割の表示
		PBoxAnt->Canvas->Pen->Color = clGreen;
		for( i = 1; i <= (GetPlusMax()+(pCalAnt->wzmax*2)); i++ ){
			GetSegPos(cx, cy, cz, i);
			CalcAntViewXY(x, y, deg, zdeg, cx, cy, cz);
			X = int((x * sc)) + Xc;
			Y = Yc - int((y * sc));
			::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
			PBoxAnt->Canvas->MoveTo(X-3, Y-3); PBoxAnt->Canvas->LineTo(X+3, Y+3);
			PBoxAnt->Canvas->MoveTo(X+3, Y-3); PBoxAnt->Canvas->LineTo(X-3, Y+3);
		}
	}
	int	YT = 0;
	PBoxAnt->Canvas->Pen->Color = clRed;
	int FH = PBoxAnt->Canvas->TextHeight("給電点");
	PBoxAnt->Canvas->TextOut(12, YT, "給電点");
	PBoxAnt->Canvas->Ellipse(3, YT + (FH/4), 10, YT + (FH/4) + 7);
	YT += FH + 2;
	for( i = 0; i < pCalAnt->cmax; i++ ){	// 給電点
		int PlusNo = Str2PlusNo(pCalAnt, pCalAnt->cdef[i].PLUS);
		if( PlusNo <= 0 ) continue;
		GetSegPos(cx, cy, cz, Plus2Seg(PlusNo));
		CalcAntViewXY(x, y, deg, zdeg, cx, cy, cz);
		X = int((x * sc)) + Xc;
		Y = Yc - int((y * sc));
		::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
		PBoxAnt->Canvas->Ellipse(X-3, Y-3, X+4, Y+4);
	}

	PBoxAnt->Canvas->Pen->Color = clRed;
	Y = YT+(FH/2);
	PBoxAnt->Canvas->MoveTo(3, Y-3); PBoxAnt->Canvas->LineTo(9, Y+3);
	PBoxAnt->Canvas->MoveTo(9, Y-3); PBoxAnt->Canvas->LineTo(3, Y+3);
	PBoxAnt->Canvas->TextOut(12, YT, "集中定数");
	if( EnbLoad->Checked ){
		for( i = 0; i < pCalAnt->lmax; i++ ){	// ロード
			int PlusNo = Str2PlusNo(pCalAnt, pCalAnt->ldef[i].PLUS);
			if( PlusNo <= 0 ) continue;
			GetSegPos(cx, cy, cz, Plus2Seg(PlusNo));
			CalcAntViewXY(x, y, deg, zdeg, cx, cy, cz);
			X = int((x * sc)) + Xc;
			Y = Yc - int((y * sc));
			::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
			PBoxAnt->Canvas->MoveTo(X-3, Y-3); PBoxAnt->Canvas->LineTo(X+3, Y+3);
			PBoxAnt->Canvas->MoveTo(X+3, Y-3); PBoxAnt->Canvas->LineTo(X-3, Y+3);
		}
	}
#if 0
	YT += FH + 2;
	PBoxAnt->Canvas->Pen->Color = clBlue;
	X = 5; Y = YT+(FH/2);
	PBoxAnt->Canvas->MoveTo(X-2, Y-2);
	PBoxAnt->Canvas->LineTo(X+2, Y-2); PBoxAnt->Canvas->LineTo(X+2, Y+2);
	PBoxAnt->Canvas->LineTo(X-2, Y+2); PBoxAnt->Canvas->LineTo(X-2, Y-2);
	PBoxAnt->Canvas->TextOut(12, YT, "組み合わせ点");
#endif

	// 電流分布の表示
	if( DspCur->Checked && res.IsCalc() ){
		int s, p, v;
		int AX=0;
		int AY=0;
		int	wno = -1;
		double Mag = double(TBarCur->Position) / 10.0;
		Mag = Mag * Mag * Mag * Mag;
		double	Vect, Cur;
		for( i = 0; i < pCalAnt->wzmax; i++ ){
			if( !pCalAnt->wzdef[i].PNo ) continue;
			s = pCalAnt->wzdef[i].Wno;
			Vect = ABS(pCalAnt->wdef[s].Z2 - pCalAnt->wdef[s].Z1);
			if( (Vect >= ABS(pCalAnt->wdef[s].Y2 - pCalAnt->wdef[s].Y1))&&
				(Vect >= ABS(pCalAnt->wdef[s].X2 - pCalAnt->wdef[s].X1)) ){
				v = 1;
			}
			else {
				v = 0;
			}
			if( exeenv.CurDir ){
				Vect = (pCalAnt->wdef[s].X2 - pCalAnt->wdef[s].X1) + (pCalAnt->wdef[s].Y2 - pCalAnt->wdef[s].Y1) + (pCalAnt->wdef[s].Z2 - pCalAnt->wdef[s].Z1);
			}
			for( s = pCalAnt->wzdef[i].SNo, p = pCalAnt->wzdef[i].PNo; s <= pCalAnt->wzdef[i].SMax; s++, p++ ){
				GetSegPos(cx, cy, cz, s);
				if( res.Cur[p-1] == NULLF ) continue;
				Cur = res.Cur[p-1] * Mag;
				if( exeenv.CurDir ){
					Cur = (Vect >= 0) ? Cur : -Cur;
				}
				else {
					Cur = ABS(Cur);
				}
				if( v ){
					cy += Cur;
					PBoxAnt->Canvas->Pen->Color = clRed;
				}
				else {
					cz += Cur;
					PBoxAnt->Canvas->Pen->Color = clBlue;
				}
				CalcAntViewXY(x, y, deg, zdeg, cx, cy, cz);
				X = int((x * sc)) + Xc;
				Y = Yc - int((y * sc));
				ClipXY(X, Y);
				if( wno != pCalAnt->wzdef[i].Wno ){
					wno = pCalAnt->wzdef[i].Wno;
				}
				else {
					::SetBkMode(PBoxAnt->Canvas->Handle, TRANSPARENT);
					PBoxAnt->Canvas->MoveTo(AX, AY); PBoxAnt->Canvas->LineTo(X, Y);
				}
				AX = X; AY = Y;
			}
		}
	}
	int SelWire = Grid2->Row - 1;
	if( (GetAntSelectionCount() > 0) && (SelWire >= 0) && (SelWire < ant.wmax) ){
		DrawWirePara(PBoxAnt->Canvas, PBoxAnt->Width, PBoxAnt->Height, &ant, SelWire);
	}
	if( (ant.StackH > 1)||(ant.StackV > 1) ){
		char bf[32];
		sprintf(bf, "%u×%u スタック", ant.StackH, ant.StackV);
		X = PBoxAnt->Width - PBoxAnt->Canvas->TextWidth(bf);
		PBoxAnt->Canvas->Font->Color = clRed;
		PBoxAnt->Canvas->TextOut(X-2, 0+2, bf);
		PBoxAnt->Canvas->Font->Color = clBlack;
	}
	PaintAntSelectionRect();
	::SetBkMode(PBoxAnt->Canvas->Handle, Sop);
	AntPaintBufferCanvas = OldAntPaintCanvas;
	this->PBoxAnt->Canvas->Draw(0, 0, AntPaintBitmap);
	}
	__finally {
		AntPaintBufferCanvas = OldAntPaintCanvas;
		delete AntPaintBitmap;
	}
}
//---------------------------------------------------------------------------
// マウスによるワイヤの選択
int __fastcall TMainWnd::SelectWire(int X, int Y)
{
	int r = FALSE;
	// ワイヤの検索
	double sc = double(TBarSC->Position) / 20.0;
	sc = sc * sc * sc * sc;
	int	Xc = int(PBoxAnt->Width/2 + (exeenv.AntXc * sc));
	int	Yc = int(PBoxAnt->Height/2 + (exeenv.AntYc * sc));
	double x, y;
	double deg = double(TBarDeg->Position);
	deg *= (PAI / 180.0);
	double zdeg = double(TBarZDeg->Position);
	zdeg *= (PAI / 180.0);
	POINT	PP, BP, EP;
	PP.x = X; PP.y = Y;
	int i;
	int mw = 9;
	int w, mi;
	for( i = 0; i < pCalAnt->wmax; i++ ){
		CalcAntViewXY(x, y, deg, zdeg, pCalAnt->wdef[i].X1, pCalAnt->wdef[i].Y1, pCalAnt->wdef[i].Z1);
		BP.x = int((x * sc)) + Xc;
		BP.y = Yc - int((y * sc));
		CalcAntViewXY(x, y, deg, zdeg, pCalAnt->wdef[i].X2, pCalAnt->wdef[i].Y2, pCalAnt->wdef[i].Z2);
		EP.x = int((x * sc)) + Xc;
		EP.y = Yc - int((y * sc));
		for( w = 1; w <= mw; w++ ){
			if( IsLine(PP, BP, EP, w) != 0 ){
				mw = w;
				mi = i;
				break;
			}
		}
	}
	if( mw != 9 ){
		if( mi >= ant.wmax ) mi %= ant.wmax;
		if( PBoxAntClickCtrl && PBoxAntClickShift ) ToggleAntElementSelection(mi);
		else if( PBoxAntClickCtrl ) ToggleAntWireSelection(mi);
		else SelectOnlyAntWire(mi);
		r = TRUE;
	}
	return r;
}
//---------------------------------------------------------------------------
// Rect selection in antenna view
int __fastcall TMainWnd::SelectWiresInRect(int X1, int Y1, int X2, int Y2)
{
	int L = (X1 < X2) ? X1 : X2;
	int R = (X1 < X2) ? X2 : X1;
	int T = (Y1 < Y2) ? Y1 : Y2;
	int B = (Y1 < Y2) ? Y2 : Y1;

	AntEditorClearSelection(AntWireSelected, AntWireSelectionCount);
	AntGizmoShowSnapVertices = FALSE;

	double sc = double(TBarSC->Position) / 20.0;
	sc = sc * sc * sc * sc;
	int	Xc = int(PBoxAnt->Width/2 + (exeenv.AntXc * sc));
	int	Yc = int(PBoxAnt->Height/2 + (exeenv.AntYc * sc));
	double x, y;
	double deg = double(TBarDeg->Position);
	deg *= (PAI / 180.0);
	double zdeg = double(TBarZDeg->Position);
	zdeg *= (PAI / 180.0);

	int first = -1;
	for( int i = 0; i < pCalAnt->wmax; i++ ){
		CalcAntViewXY(x, y, deg, zdeg, pCalAnt->wdef[i].X1, pCalAnt->wdef[i].Y1, pCalAnt->wdef[i].Z1);
		int sx1 = int((x * sc)) + Xc;
		int sy1 = Yc - int((y * sc));
		CalcAntViewXY(x, y, deg, zdeg, pCalAnt->wdef[i].X2, pCalAnt->wdef[i].Y2, pCalAnt->wdef[i].Z2);
		int sx2 = int((x * sc)) + Xc;
		int sy2 = Yc - int((y * sc));
		if( AntSegmentIntersectsRect(sx1, sy1, sx2, sy2, L, T, R, B) ){
			int sw = ant.wmax ? (i % ant.wmax) : i;
			if( (sw >= 0) && (sw < ant.wmax) && !AntWireSelected[sw] ){
				AntWireSelected[sw] = TRUE;
				AntWireSelectionCount++;
				if( first < 0 ) first = sw;
			}
		}
	}

	if( AntWireSelectionCount > 0 ){
		Grid2->Row = first + 1;
		Grid2->SetFocus();
	}
	else {
		AntEditorDeselectSelection(AntWireSelected, AntWireSelectionCount);
		Grid2->EditorMode = FALSE;
	}
	Grid2->Invalidate();
	PBoxAnt->Invalidate();
	return (AntWireSelectionCount > 0) ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
// Antenna view mouse action setting
int __fastcall TMainWnd::GetAntMouseAction(TMouseButton Button)
{
	if( Button == mbLeft ) return exeenv.AntMouseLeft;
	if( Button == mbMiddle ) return exeenv.AntMouseMiddle;
	return ANT_MOUSE_NONE;
}
//---------------------------------------------------------------------------
double __fastcall TMainWnd::GetAntViewScale(void)
{
	return AntViewScaleFromTrack(TBarSC->Position);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SetTrackBarPosition(TTrackBar *Bar, int Pos)
{
	if( Pos < Bar->Min ) Pos = Bar->Min;
	if( Pos > Bar->Max ) Pos = Bar->Max;
	if( Bar->Position != Pos ) Bar->Position = Pos;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SetTrackBarPositionWrapped(TTrackBar *Bar, int Pos)
{
	int range = Bar->Max - Bar->Min + 1;
	if( range <= 0 ){
		SetTrackBarPosition(Bar, Pos);
		return;
	}
	while( Pos < Bar->Min ) Pos += range;
	while( Pos > Bar->Max ) Pos -= range;
	if( Bar->Position != Pos ) Bar->Position = Pos;
}
//---------------------------------------------------------------------------
double __fastcall TMainWnd::GetAntViewUnitScale(void)
{
	return AntViewUnitScale(exeenv.RmdSel, exeenv.MmSel);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::CalcAntViewXY(double &x, double &y, double deg, double zdeg, double X, double Y, double Z)
{
	AntViewProject(x, y, deg, zdeg, GetAntViewUnitScale(), X, Y, Z);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntViewPan(int DX, int DY)
{
	double sc = GetAntViewScale();
	if( sc <= 0.0 ) return;
	exeenv.AntXc += double(DX) / sc;
	exeenv.AntYc += double(DY) / sc;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntViewRotate(int DX, int DY)
{
	AntViewTrackBarChanging = TRUE;
	SetTrackBarPositionWrapped(TBarDeg, TBarDeg->Position + DX);
	if( exeenv.Ant3D ){
		SetTrackBarPosition(TBarZDeg, TBarZDeg->Position - DY);
	}
	AntViewTrackBarChanging = FALSE;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntViewRotateDrag(int X, int Y)
{
	int dx = X - PBoxAntMX;
	int dy = Y - PBoxAntMY;
	int deg = PBoxAntStartDeg + AntViewRoundMouseAngle(AntViewMouseDragToAngle(dx, PBoxAnt->Width));
	AntViewTrackBarChanging = TRUE;
	SetTrackBarPositionWrapped(TBarDeg, deg);
	if( exeenv.Ant3D ){
		int zdeg = PBoxAntStartZDeg - AntViewRoundMouseAngle(AntViewMouseDragToAngle(dy, PBoxAnt->Height));
		SetTrackBarPosition(TBarZDeg, zdeg);
	}
	AntViewTrackBarChanging = FALSE;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::ClearAntRedo(void)
{
	AntRedoList->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PushAntUndo(void)
{
	AnsiString snap;
	SaveAntStrings(snap);
	if( AntUndoList->Count && (AntUndoList->Strings[AntUndoList->Count - 1] == snap) ) return;
	AntUndoList->Add(snap);
	while( AntUndoList->Count > 64 ) AntUndoList->Delete(0);
	ClearAntRedo();
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::RestoreAntSnapshot(TStringList *From, TStringList *To)
{
	if( From->Count <= 0 ) return FALSE;
	AnsiString cur;
	SaveAntStrings(cur);
	To->Add(cur);
	while( To->Count > 64 ) To->Delete(0);

	int n = From->Count - 1;
	AnsiString snap = From->Strings[n];
	From->Delete(n);
	LoadAntStrings(snap);
	Grid2->EditorMode = FALSE;
	Grid3->EditorMode = FALSE;
	Grid4->EditorMode = FALSE;
	SetAntDef();
	ant.Edit = ant.Flag = 1;
	exeenv.CalcLog = 1;
	SetStackAnt();
	res.ClearBWC();
	Grid2->Invalidate();
	Grid3->Invalidate();
	Grid4->Invalidate();
	UpdateAllViews();
	RequestAutoCalc(FALSE);
	return TRUE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::UndoAntEdit(void)
{
	if( RestoreAntSnapshot(AntUndoList, AntRedoList) != TRUE ) ::MessageBeep(MB_ICONEXCLAMATION);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::RedoAntEdit(void)
{
	if( RestoreAntSnapshot(AntRedoList, AntUndoList) != TRUE ) ::MessageBeep(MB_ICONEXCLAMATION);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::DeleteSelectedAntWires(void)
{
	if( GetAntSelectionCount() <= 0 ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	PushAntUndo();
	int firstDeleted = -1;
	if( AntEditorDeleteSelectedWires(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, firstDeleted) != TRUE ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	AntGizmoShowSnapVertices = FALSE;
	UpdateCount();
	Grid2->RowCount = ant.wmax + 2;
	if( ant.wmax ){
		if( firstDeleted >= ant.wmax ) firstDeleted = ant.wmax - 1;
		Grid2->Row = firstDeleted + 1;
	}
	else {
		Grid2->Row = 1;
	}
	Grid2->Invalidate();
	PBoxAnt->Invalidate();
	UpdateAntPreview();
	RequestAutoCalc(FALSE);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::ClearAntWireSelection(void)
{
	AntEditorClearSelection(AntWireSelected, AntWireSelectionCount);
	AntGizmoShowSnapVertices = FALSE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::DeselectAntWireSelection(void)
{
	AntEditorDeselectSelection(AntWireSelected, AntWireSelectionCount);
	AntGizmoShowSnapVertices = FALSE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SelectOnlyAntWire(int Wire)
{
	ClearAntWireSelection();
	if( (Wire >= 0) && (Wire < ant.wmax) ){
		AntWireSelected[Wire] = TRUE;
		AntWireSelectionCount = 1;
		Grid2->Row = Wire + 1;
		Grid2->SetFocus();
	}
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::ToggleAntWireSelection(int Wire)
{
	if( (Wire < 0) || (Wire >= ant.wmax) ) return;
	if( AntWireSelectionCount < 0 ){
		AntWireSelectionCount = 0;
	}
	else if( AntWireSelectionCount == 0 ){
		int cur = Grid2->Row - 1;
		if( (cur >= 0) && (cur < ant.wmax) && (cur != Wire) ){
			AntWireSelected[cur] = TRUE;
			AntWireSelectionCount = 1;
		}
	}
	if( AntWireSelected[Wire] ){
		AntWireSelected[Wire] = FALSE;
		if( AntWireSelectionCount > 0 ) AntWireSelectionCount--;
	}
	else {
		AntWireSelected[Wire] = TRUE;
		AntWireSelectionCount++;
		Grid2->Row = Wire + 1;
		Grid2->SetFocus();
	}
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::ToggleAntElementSelection(int Wire)
{
	if( (Wire < 0) || (Wire >= ant.wmax) ) return;

	CWGroupList list;
	list.SetWire(&ant);
	CWGroup *gp = list.GetCW(Wire);
	if( (gp == NULL) || (gp->bcnt <= 0) ){
		ToggleAntWireSelection(Wire);
		return;
	}

	int cur = Grid2->Row - 1;
	int curInElement = FALSE;
	WREF *rp;
	int n;
	for( rp = gp->pB, n = gp->bcnt; n; n--, rp++ ){
		if( rp->ref == cur ){
			curInElement = TRUE;
			break;
		}
	}

	if( AntWireSelectionCount < 0 ){
		AntWireSelectionCount = 0;
	}
	else if( AntWireSelectionCount == 0 ){
		if( (cur >= 0) && (cur < ant.wmax) && !curInElement ){
			AntWireSelected[cur] = TRUE;
			AntWireSelectionCount = 1;
		}
	}

	int allSelected = TRUE;
	for( rp = gp->pB, n = gp->bcnt; n; n--, rp++ ){
		int w = rp->ref;
		if( (w < 0) || (w >= ant.wmax) ) continue;
		if( !AntWireSelected[w] ){
			allSelected = FALSE;
			break;
		}
	}

	if( allSelected ){
		for( rp = gp->pB, n = gp->bcnt; n; n--, rp++ ){
			int w = rp->ref;
			if( (w < 0) || (w >= ant.wmax) ) continue;
			if( AntWireSelected[w] ){
				AntWireSelected[w] = FALSE;
				if( AntWireSelectionCount > 0 ) AntWireSelectionCount--;
			}
		}
		if( AntWireSelectionCount <= 0 ){
			DeselectAntWireSelection();
			Grid2->EditorMode = FALSE;
		}
	}
	else {
		int first = -1;
		for( rp = gp->pB, n = gp->bcnt; n; n--, rp++ ){
			int w = rp->ref;
			if( (w < 0) || (w >= ant.wmax) ) continue;
			if( !AntWireSelected[w] ){
				AntWireSelected[w] = TRUE;
				AntWireSelectionCount++;
			}
			if( first < 0 ) first = w;
		}
		if( first >= 0 ){
			Grid2->Row = first + 1;
			Grid2->SetFocus();
		}
	}
	Grid2->Invalidate();
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SelectAntWireClick(int X, int Y)
{
	if( SelectWire(X, Y) == TRUE ){
		PBoxAnt->Invalidate();
	}
	else {
		DeselectAntWireSelection();
		Grid2->EditorMode = FALSE;
		Grid2->Invalidate();
		PBoxAnt->Invalidate();
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SelectAllAntWires(void)
{
	if( ant.wmax <= 0 ) return;
	for( int i = 0; i < ant.wmax; i++ ) AntWireSelected[i] = TRUE;
	AntWireSelectionCount = ant.wmax;
	Grid2->Row = 1;
	Grid2->SetFocus();
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::GetAntSelectionCount(void)
{
	return AntEditorSelectionCount(&ant, AntWireSelectionCount, Grid2->Row - 1);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::IsAntWireSelected(int Wire)
{
	return AntEditorIsWireSelected(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, Wire);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::GetAntSelectionCenter(double &X, double &Y, double &Z)
{
	return AntEditorSelectionCenter(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, X, Y, Z);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::CopyAntWires(void)
{
	if( AntEditorCopySelectedWires(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, AntWireClipboard, AntWireClipboardCount) != TRUE ){
		::MessageBeep(MB_ICONEXCLAMATION);
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PasteAntWires(void)
{
	if( AntWireClipboardCount <= 0 ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	if( ant.wmax + AntWireClipboardCount > WMAX ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	PushAntUndo();
	int first = 0;
	if( AntEditorPasteWires(&ant, AntWireSelected, AntWireSelectionCount, AntWireClipboard, AntWireClipboardCount, first) != TRUE ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	Grid2->RowCount = ant.wmax + 2;
	Grid2->Row = first + 1;
	UpdateAntData();
	Grid2->Row = first + 1;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::MirrorSelectedWires(int Axis)
{
	if( GetAntSelectionCount() <= 0 ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	PushAntUndo();
	if( AntEditorMirrorSelectedWires(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, Axis) != TRUE ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	UpdateAntData();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::MirrorSelectedXClick(TObject *Sender)
{
	MirrorSelectedWires(ANT_GIZMO_AXIS_X);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::MirrorSelectedYClick(TObject *Sender)
{
	MirrorSelectedWires(ANT_GIZMO_AXIS_Y);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::MirrorSelectedZClick(TObject *Sender)
{
	MirrorSelectedWires(ANT_GIZMO_AXIS_Z);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AlignSelectedWiresToOrigin(int Axis)
{
	if( GetAntSelectionCount() <= 0 ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	PushAntUndo();
	if( AntEditorAlignSelectedWiresToOrigin(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, Axis) != TRUE ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	UpdateAntData();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::RotateSelectedWires90(void)
{
	if( GetAntSelectionCount() <= 0 ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	PushAntUndo();
	if( AntEditorRotateSelectedWires90(&ant, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1) != TRUE ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	UpdateAntData();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AlignSelectedX0Click(TObject *Sender)
{
	AlignSelectedWiresToOrigin(ANT_GIZMO_AXIS_X);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AlignSelectedY0Click(TObject *Sender)
{
	AlignSelectedWiresToOrigin(ANT_GIZMO_AXIS_Y);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AlignSelectedZ0Click(TObject *Sender)
{
	AlignSelectedWiresToOrigin(ANT_GIZMO_AXIS_Z);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::RotateSelected90Click(TObject *Sender)
{
	RotateSelectedWires90();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::ToggleAntRotationGizmoClick(TObject *Sender)
{
	AntRotationGizmoVisible = AntRotationGizmoVisible ? FALSE : TRUE;
	UpdateAntDrawControls();
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::CreateAntDrawControls(void)
{
	if( AntDrawBtn != NULL ) return;

	AntDrawBtn = new TButton(this);
	AntDrawBtn->Parent = TabSheet2;
	AntDrawBtn->Caption = "Draw";
	AntDrawBtn->TabStop = false;
	AntDrawBtn->OnClick = AntDrawWireToggle;

	AntDrawXYBtn = new TButton(this);
	AntDrawXYBtn->Parent = TabSheet2;
	AntDrawXYBtn->Caption = "XY";
	AntDrawXYBtn->TabStop = false;
	AntDrawXYBtn->OnClick = AntDrawPlaneClick;

	AntDrawXZBtn = new TButton(this);
	AntDrawXZBtn->Parent = TabSheet2;
	AntDrawXZBtn->Caption = "XZ";
	AntDrawXZBtn->TabStop = false;
	AntDrawXZBtn->OnClick = AntDrawPlaneClick;

	AntDrawYZBtn = new TButton(this);
	AntDrawYZBtn->Parent = TabSheet2;
	AntDrawYZBtn->Caption = "YZ";
	AntDrawYZBtn->TabStop = false;
	AntDrawYZBtn->OnClick = AntDrawPlaneClick;

	AntDrawCancelBtn = new TButton(this);
	AntDrawCancelBtn->Parent = TabSheet2;
	AntDrawCancelBtn->Caption = "Cancel";
	AntDrawCancelBtn->TabStop = false;
	AntDrawCancelBtn->OnClick = AntDrawCancelClick;

	AntAlignX0Btn = new TButton(this);
	AntAlignX0Btn->Parent = TabSheet2;
	AntAlignX0Btn->Caption = "X0";
	AntAlignX0Btn->TabStop = false;
	AntAlignX0Btn->Hint = "Move selected wires to X=0";
	AntAlignX0Btn->ShowHint = true;
	AntAlignX0Btn->OnClick = AlignSelectedX0Click;

	AntAlignY0Btn = new TButton(this);
	AntAlignY0Btn->Parent = TabSheet2;
	AntAlignY0Btn->Caption = "Y0";
	AntAlignY0Btn->TabStop = false;
	AntAlignY0Btn->Hint = "Move selected wires to Y=0";
	AntAlignY0Btn->ShowHint = true;
	AntAlignY0Btn->OnClick = AlignSelectedY0Click;

	AntAlignZ0Btn = new TButton(this);
	AntAlignZ0Btn->Parent = TabSheet2;
	AntAlignZ0Btn->Caption = "Z0";
	AntAlignZ0Btn->TabStop = false;
	AntAlignZ0Btn->Hint = "Move selected wires to Z=0";
	AntAlignZ0Btn->ShowHint = true;
	AntAlignZ0Btn->OnClick = AlignSelectedZ0Click;

	AntRotate90Btn = new TButton(this);
	AntRotate90Btn->Parent = TabSheet2;
	AntRotate90Btn->Caption = "R90";
	AntRotate90Btn->TabStop = false;
	AntRotate90Btn->Hint = "Rotate selected wires 90 degrees";
	AntRotate90Btn->ShowHint = true;
	AntRotate90Btn->OnClick = RotateSelected90Click;

	AntRotationGizmoBtn = new TButton(this);
	AntRotationGizmoBtn->Parent = TabSheet2;
	AntRotationGizmoBtn->Caption = "Rot";
	AntRotationGizmoBtn->TabStop = false;
	AntRotationGizmoBtn->Hint = "Show/hide rotation gizmo";
	AntRotationGizmoBtn->ShowHint = true;
	AntRotationGizmoBtn->OnClick = ToggleAntRotationGizmoClick;

	LayoutAntDrawControls();
	UpdateAntDrawControls();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::LayoutAntDrawControls(void)
{
	if( (AntDrawBtn == NULL) || (AntAlignX0Btn == NULL) || (AntAlignZ0Btn == NULL) || (AntRotationGizmoBtn == NULL) || (AllViewBtn == NULL) || (OrgBtn == NULL) ) return;

	const int gap = 4;
	const int drawW = 54;
	const int planeW = 32;
	const int cancelW = 50;
	const int alignW = 36;
	const int rotateW = 44;
	const int rotateGizmoW = 40;
	int top = OrgBtn->Top;
	int h = OrgBtn->Height;

	if( QuadMode && (PanelTopRight != NULL) ){
		top = 2;
		h = 25;
		int right = PanelTopRight->ClientWidth - gap;
		OrgBtn->SetBounds(right - 91, top, 91, h);
		right -= 91 + gap;
		AllViewBtn->SetBounds(right - 91, top, 91, h);

		OrgBtn->Font->Assign(Font);
		AllViewBtn->Font->Assign(Font);
		AntDrawBtn->Font->Assign(Font);
		AntDrawXYBtn->Font->Assign(Font);
		AntDrawXZBtn->Font->Assign(Font);
		AntDrawYZBtn->Font->Assign(Font);
		AntDrawCancelBtn->Font->Assign(Font);
		AntAlignX0Btn->Font->Assign(Font);
		AntAlignY0Btn->Font->Assign(Font);
		AntAlignZ0Btn->Font->Assign(Font);
		AntRotate90Btn->Font->Assign(Font);
		AntRotationGizmoBtn->Font->Assign(Font);
	}

	int total = drawW + gap + planeW + gap + planeW + gap + planeW + gap +
		cancelW + gap + alignW + gap + alignW + gap + alignW + gap + rotateW + gap + rotateGizmoW;
	int left = AllViewBtn->Left - gap - total;
	if( left < 2 ) left = 2;

	AntDrawBtn->SetBounds(left, top, drawW, h);
	left += drawW + gap;
	AntDrawXYBtn->SetBounds(left, top, planeW, h);
	left += planeW + gap;
	AntDrawXZBtn->SetBounds(left, top, planeW, h);
	left += planeW + gap;
	AntDrawYZBtn->SetBounds(left, top, planeW, h);
	left += planeW + gap;
	AntDrawCancelBtn->SetBounds(left, top, cancelW, h);
	left += cancelW + gap;
	AntAlignX0Btn->SetBounds(left, top, alignW, h);
	left += alignW + gap;
	AntAlignY0Btn->SetBounds(left, top, alignW, h);
	left += alignW + gap;
	AntAlignZ0Btn->SetBounds(left, top, alignW, h);
	left += alignW + gap;
	AntRotate90Btn->SetBounds(left, top, rotateW, h);
	left += rotateW + gap;
	AntRotationGizmoBtn->SetBounds(left, top, rotateGizmoW, h);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::UpdateAntDrawControls(void)
{
	if( AntDrawBtn == NULL ) return;

	bool enabled = (!CalcBusyDepth && exeenv.Ant3D) ? true : false;
	bool selected = (!CalcBusyDepth && (GetAntSelectionCount() > 0)) ? true : false;
	AntDrawBtn->Enabled = enabled;
	AntDrawXYBtn->Enabled = enabled;
	AntDrawXZBtn->Enabled = enabled;
	AntDrawYZBtn->Enabled = enabled;
	AntDrawCancelBtn->Enabled = enabled && AntDrawMode;
	AntAlignX0Btn->Enabled = selected;
	AntAlignY0Btn->Enabled = selected;
	AntAlignZ0Btn->Enabled = selected;
	AntRotate90Btn->Enabled = selected;
	AntRotationGizmoBtn->Enabled = enabled;

	AntDrawBtn->Caption = AntDrawMode ? "[Draw]" : "Draw";
	AntDrawXYBtn->Caption = (AntDrawPlane == ANT_DRAW_PLANE_XY) ? "[XY]" : "XY";
	AntDrawXZBtn->Caption = (AntDrawPlane == ANT_DRAW_PLANE_XZ) ? "[XZ]" : "XZ";
	AntDrawYZBtn->Caption = (AntDrawPlane == ANT_DRAW_PLANE_YZ) ? "[YZ]" : "YZ";
	AntRotationGizmoBtn->Caption = AntRotationGizmoVisible ? "[Rot]" : "Rot";

	if( KAntDrawWire != NULL ){
		KAntDrawWire->Checked = AntDrawMode ? true : false;
		KAntDrawWire->Enabled = enabled;
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SetAntDrawPlane(int Plane)
{
	if( (Plane < ANT_DRAW_PLANE_XY) || (Plane > ANT_DRAW_PLANE_YZ) ) return;
	if( AntDrawPlane != Plane ){
		AntDrawPlane = Plane;
		AntDrawActive = FALSE;
		AntDrawSnapVisible = FALSE;
	}
	UpdateAntDrawControls();
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntDrawPlaneClick(TObject *Sender)
{
	if( Sender == AntDrawXYBtn ) SetAntDrawPlane(ANT_DRAW_PLANE_XY);
	else if( Sender == AntDrawXZBtn ) SetAntDrawPlane(ANT_DRAW_PLANE_XZ);
	else if( Sender == AntDrawYZBtn ) SetAntDrawPlane(ANT_DRAW_PLANE_YZ);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntDrawCancelClick(TObject *Sender)
{
	if( AntDrawActive ){
		AntDrawActive = FALSE;
		AntDrawSnapVisible = FALSE;
		PBoxAnt->Invalidate();
	}
	else {
		SetAntDrawMode(FALSE);
	}
	UpdateAntDrawControls();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::SetAntDrawMode(int Enabled)
{
	if( Enabled && !exeenv.Ant3D ){
		::MessageBeep(MB_ICONEXCLAMATION);
		Enabled = FALSE;
	}
	AntDrawMode = Enabled ? TRUE : FALSE;
	AntDrawActive = FALSE;
	AntDrawSnapVisible = FALSE;
	PBoxAntDragButton = -1;
	PBoxAntDragAction = ANT_MOUSE_NONE;
	PBoxAnt->Cursor = AntDrawMode ? crCross : crDefault;
	UpdateAntDrawControls();
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntDrawWireToggle(TObject *Sender)
{
	SetAntDrawMode(!AntDrawMode);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::AntViewPointToXY(int X, int Y, double &WX, double &WY, double &WZ)
{
	if( !exeenv.Ant3D ) return FALSE;
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();
	TAntDrawPlaneState plane;
	plane.Plane = AntDrawPlane;
	plane.Active = AntDrawActive;
	plane.X1 = AntDrawX1;
	plane.Y1 = AntDrawY1;
	plane.Z1 = AntDrawZ1;
	return AntViewScreenToPlane(view, plane, X, Y, WX, WY, WZ);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntWorldToScreen(double WX, double WY, double WZ, int &X, int &Y)
{
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();
	AntViewWorldToScreen(view, WX, WY, WZ, X, Y);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::SnapAntDrawPoint(int X, int Y, double &WX, double &WY, double &WZ)
{
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();
	AntDrawSnapVisible = FALSE;
	int found = AntViewFindSnapPoint(&ant, view, X, Y, WX, WY, WZ);
	if( found ){
		AntDrawSnapVisible = TRUE;
		AntDrawSnapX = WX;
		AntDrawSnapY = WY;
		AntDrawSnapZ = WZ;
	}
	return found;
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::GetAntGizmoAxisScreen(double WX, double WY, double WZ, int Axis, int Len,
	int &X1, int &Y1, int &X2, int &Y2, double &DX, double &DY)
{
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();
	return AntViewGizmoAxisScreen(view, WX, WY, WZ, Axis, Len, X1, Y1, X2, Y2, DX, DY);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::GetAntGizmoRingRadius(double WX, double WY, double WZ, int Axis, double &Radius)
{
	int cx, cy, x1, y1, x2, y2;
	AntWorldToScreen(WX, WY, WZ, cx, cy);
	switch( Axis ){
		case ANT_GIZMO_AXIS_X:
			AntWorldToScreen(WX, WY + 1.0, WZ, x1, y1);
			AntWorldToScreen(WX, WY, WZ + 1.0, x2, y2);
			break;
		case ANT_GIZMO_AXIS_Y:
			AntWorldToScreen(WX + 1.0, WY, WZ, x1, y1);
			AntWorldToScreen(WX, WY, WZ + 1.0, x2, y2);
			break;
		case ANT_GIZMO_AXIS_Z:
			AntWorldToScreen(WX + 1.0, WY, WZ, x1, y1);
			AntWorldToScreen(WX, WY + 1.0, WZ, x2, y2);
			break;
		default:
			return FALSE;
	}
	double dx1 = double(x1 - cx);
	double dy1 = double(y1 - cy);
	double dx2 = double(x2 - cx);
	double dy2 = double(y2 - cy);
	double d1 = sqrt((dx1 * dx1) + (dy1 * dy1));
	double d2 = sqrt((dx2 * dx2) + (dy2 * dy2));
	double d = (d1 > d2) ? d1 : d2;
	if( d < 0.1 ) return FALSE;
	Radius = 54.0 / d;
	return TRUE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntRotationGizmo(double WX, double WY, double WZ, int Endpoint)
{
	if( !AntRotationGizmoVisible ) return;
	TCanvas *Canvas = AntPaintCanvas();
	for( int axis = 0; axis < 3; axis++ ){
		double radius;
		if( GetAntGizmoRingRadius(WX, WY, WZ, axis, radius) != TRUE ) continue;
		Canvas->Pen->Color = AntGizmoAxisColor(axis, FALSE);
		Canvas->Pen->Style = psSolid;
		Canvas->Pen->Width = (AntGizmoDrag && (AntGizmoMode == ANT_GIZMO_MODE_ROTATE) &&
			(AntGizmoEndpoint == Endpoint) && (AntGizmoAxis == axis)) ? 3 : 1;

		int first = TRUE;
		for( int i = 0; i <= 72; i++ ){
			double angle = (double(i) * PAI2) / 72.0;
			double rx, ry, rz;
			AntGizmoRingPoint(axis, WX, WY, WZ, radius, angle, rx, ry, rz);
			int sx, sy;
			AntWorldToScreen(rx, ry, rz, sx, sy);
			if( first ){
				Canvas->MoveTo(sx, sy);
				first = FALSE;
			}
			else {
				Canvas->LineTo(sx, sy);
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntEditGizmo(void)
{
	if( AntDrawMode || !exeenv.Ant3D ) return;
	int SelCount = GetAntSelectionCount();
	if( SelCount <= 0 ) return;

	TCanvas *Canvas = AntPaintCanvas();
	TColor oldColor = Canvas->Pen->Color;
	TPenStyle oldStyle = Canvas->Pen->Style;
	int oldWidth = Canvas->Pen->Width;
	TColor oldBrush = Canvas->Brush->Color;
	TBrushStyle oldBrushStyle = Canvas->Brush->Style;

	if( SelCount > 1 ){
		double wx, wy, wz;
		if( GetAntSelectionCenter(wx, wy, wz) != TRUE ) return;
		int sx, sy;
		AntWorldToScreen(wx, wy, wz, sx, sy);
		Canvas->Pen->Color = clBlack;
		Canvas->Pen->Style = psSolid;
		Canvas->Pen->Width = 1;
		Canvas->Brush->Style = bsClear;
		Canvas->Rectangle(sx-5, sy-5, sx+6, sy+6);
		PaintAntRotationGizmo(wx, wy, wz, 3);
		for( int axis = 0; axis < 3; axis++ ){
			int x1, y1, x2, y2;
			double dx, dy;
			if( GetAntGizmoAxisScreen(wx, wy, wz, axis, 46, x1, y1, x2, y2, dx, dy) != TRUE ) continue;
			switch( axis ){
				case ANT_GIZMO_AXIS_X: Canvas->Pen->Color = clRed; break;
				case ANT_GIZMO_AXIS_Y: Canvas->Pen->Color = clGreen; break;
				case ANT_GIZMO_AXIS_Z: Canvas->Pen->Color = clBlue; break;
			}
			Canvas->Pen->Width = (AntGizmoDrag && (AntGizmoMode == ANT_GIZMO_MODE_MOVE) &&
				(AntGizmoEndpoint == 3) && (AntGizmoAxis == axis)) ? 3 : 2;
			AntViewDrawScreenArrow(Canvas, x1, y1, x2, y2);
		}
		PaintAntGizmoMoveDistance();

		Canvas->Pen->Color = oldColor;
		Canvas->Pen->Style = oldStyle;
		Canvas->Pen->Width = oldWidth;
		Canvas->Brush->Color = oldBrush;
		Canvas->Brush->Style = oldBrushStyle;
		return;
	}

	int w = Grid2->Row - 1;
	if( (w < 0) || (w >= ant.wmax) ) return;
	WDEF *wp = &ant.wdef[w];
	for( int endp = 0; endp < 3; endp++ ){
		double wx = endp ? wp->X2 : wp->X1;
		double wy = endp ? wp->Y2 : wp->Y1;
		double wz = endp ? wp->Z2 : wp->Z1;
		if( endp == 2 ){
			wx = (wp->X1 + wp->X2) / 2.0;
			wy = (wp->Y1 + wp->Y2) / 2.0;
			wz = (wp->Z1 + wp->Z2) / 2.0;
		}
		int sx, sy;
		AntWorldToScreen(wx, wy, wz, sx, sy);
		Canvas->Pen->Color = (endp == 2) ? clGray : clBlack;
		Canvas->Pen->Style = psSolid;
		Canvas->Pen->Width = 1;
		Canvas->Brush->Style = bsClear;
		Canvas->Ellipse(sx-4, sy-4, sx+5, sy+5);
		if( endp == 2 ) PaintAntRotationGizmo(wx, wy, wz, endp);

		for( int axis = 0; axis < 3; axis++ ){
			int x1, y1, x2, y2;
			double dx, dy;
			if( GetAntGizmoAxisScreen(wx, wy, wz, axis, 42, x1, y1, x2, y2, dx, dy) != TRUE ) continue;
			Canvas->Pen->Color = AntGizmoAxisColor(axis, endp == 2);
			Canvas->Pen->Width = (AntGizmoDrag && (AntGizmoMode == ANT_GIZMO_MODE_MOVE) && (AntGizmoWire == w) &&
				(AntGizmoEndpoint == endp) && (AntGizmoAxis == axis)) ? 3 : 2;
			AntViewDrawScreenArrow(Canvas, x1, y1, x2, y2);
		}
	}

	PaintAntGizmoMoveDistance();

	Canvas->Pen->Color = oldColor;
	Canvas->Pen->Style = oldStyle;
	Canvas->Pen->Width = oldWidth;
	Canvas->Brush->Color = oldBrush;
	Canvas->Brush->Style = oldBrushStyle;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntSnapVertices(void)
{
	if( !AntGizmoShowSnapVertices ) return;
	TCanvas *Canvas = AntPaintCanvas();
	TColor oldColor = Canvas->Pen->Color;
	TColor oldBrush = Canvas->Brush->Color;
	TBrushStyle oldBrushStyle = Canvas->Brush->Style;
	Canvas->Pen->Color = clFuchsia;
	Canvas->Brush->Color = clFuchsia;
	Canvas->Brush->Style = bsSolid;
	for( int i = 0; i < ant.wmax; i++ ){
		if( IsAntWireSelected(i) ) continue;
		int x, y;
		AntWorldToScreen(ant.wdef[i].X1, ant.wdef[i].Y1, ant.wdef[i].Z1, x, y);
		Canvas->Ellipse(x-3, y-3, x+4, y+4);
		AntWorldToScreen(ant.wdef[i].X2, ant.wdef[i].Y2, ant.wdef[i].Z2, x, y);
		Canvas->Ellipse(x-3, y-3, x+4, y+4);
	}
	Canvas->Pen->Color = oldColor;
	Canvas->Brush->Color = oldBrush;
	Canvas->Brush->Style = oldBrushStyle;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntDrawSnapPoint(void)
{
	if( !AntDrawMode || !AntDrawSnapVisible ) return;
	int x, y;
	AntWorldToScreen(AntDrawSnapX, AntDrawSnapY, AntDrawSnapZ, x, y);
	TCanvas *Canvas = AntPaintCanvas();
	TColor oldPen = Canvas->Pen->Color;
	TColor oldBrush = Canvas->Brush->Color;
	TPenStyle oldStyle = Canvas->Pen->Style;
	int oldWidth = Canvas->Pen->Width;
	TBrushStyle oldBrushStyle = Canvas->Brush->Style;

	Canvas->Pen->Color = clBlue;
	Canvas->Pen->Style = psSolid;
	Canvas->Pen->Width = 2;
	Canvas->Brush->Color = clAqua;
	Canvas->Brush->Style = bsSolid;
	Canvas->Ellipse(x-5, y-5, x+6, y+6);
	Canvas->MoveTo(x-8, y);
	Canvas->LineTo(x+9, y);
	Canvas->MoveTo(x, y-8);
	Canvas->LineTo(x, y+9);

	Canvas->Pen->Color = oldPen;
	Canvas->Brush->Color = oldBrush;
	Canvas->Pen->Style = oldStyle;
	Canvas->Pen->Width = oldWidth;
	Canvas->Brush->Style = oldBrushStyle;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntSelectionRect(void)
{
	if( !PBoxAntSelectRect || !PBoxAntDragMoved ) return;

	int L = (PBoxAntMX < PBoxAntRectX) ? PBoxAntMX : PBoxAntRectX;
	int R = (PBoxAntMX < PBoxAntRectX) ? PBoxAntRectX : PBoxAntMX;
	int T = (PBoxAntMY < PBoxAntRectY) ? PBoxAntMY : PBoxAntRectY;
	int B = (PBoxAntMY < PBoxAntRectY) ? PBoxAntRectY : PBoxAntMY;

	TCanvas *Canvas = AntPaintCanvas();
	TColor oldPen = Canvas->Pen->Color;
	TPenStyle oldStyle = Canvas->Pen->Style;
	int oldWidth = Canvas->Pen->Width;
	TBrushStyle oldBrushStyle = Canvas->Brush->Style;
	Canvas->Pen->Color = (TColor)RGB(40, 110, 210);
	Canvas->Pen->Style = psDot;
	Canvas->Pen->Width = 1;
	Canvas->Brush->Style = bsClear;
	Canvas->Rectangle(L, T, R + 1, B + 1);
	Canvas->Pen->Color = oldPen;
	Canvas->Pen->Style = oldStyle;
	Canvas->Pen->Width = oldWidth;
	Canvas->Brush->Style = oldBrushStyle;
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::FindAntSnapVertex(int X, int Y, double &WX, double &WY, double &WZ)
{
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();
	return AntViewFindSnapVertex(&ant, view, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, X, Y, WX, WY, WZ);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::FindAntSnapEdge(int X, int Y, double &WX, double &WY, double &WZ)
{
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();
	return AntViewFindSnapEdge(&ant, view, AntWireSelected, AntWireSelectionCount, Grid2->Row - 1, X, Y, WX, WY, WZ);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::HitAntRotationGizmo(double WX, double WY, double WZ, int Endpoint,
	int X, int Y, int &HitEndpoint, int &Axis, int &Mode, double &Best)
{
	if( !AntRotationGizmoVisible ) return FALSE;
	int found = FALSE;
	for( int axis = 0; axis < 3; axis++ ){
		double radius;
		if( GetAntGizmoRingRadius(WX, WY, WZ, axis, radius) != TRUE ) continue;
		int first = TRUE;
		int px = 0;
		int py = 0;
		for( int i = 0; i <= 72; i++ ){
			double angle = (double(i) * PAI2) / 72.0;
			double rx, ry, rz;
			AntGizmoRingPoint(axis, WX, WY, WZ, radius, angle, rx, ry, rz);
			int sx, sy;
			AntWorldToScreen(rx, ry, rz, sx, sy);
			if( first ){
				first = FALSE;
			}
			else {
				double d = AntViewSqDistPointToSegment(X, Y, px, py, sx, sy);
				if( d < Best ){
					Best = d;
					HitEndpoint = Endpoint;
					Axis = axis;
					Mode = ANT_GIZMO_MODE_ROTATE;
					found = TRUE;
				}
			}
			px = sx;
			py = sy;
		}
	}
	return found;
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::HitAntEditGizmo(int X, int Y, int &Endpoint, int &Axis, int &Mode)
{
	if( AntDrawMode || !exeenv.Ant3D ) return FALSE;
	int SelCount = GetAntSelectionCount();
	if( SelCount <= 0 ) return FALSE;

	double best = (8.0 * 8.0) + 1.0;
	int found = FALSE;
	Mode = ANT_GIZMO_MODE_MOVE;
	if( SelCount > 1 ){
		double wx, wy, wz;
		if( GetAntSelectionCenter(wx, wy, wz) == TRUE ){
			int sx, sy;
			AntWorldToScreen(wx, wy, wz, sx, sy);
			int mdx = sx - X;
			int mdy = sy - Y;
			if( ((mdx * mdx) + (mdy * mdy)) <= (7 * 7) ){
				Endpoint = 3;
				Axis = -1;
				Mode = ANT_GIZMO_MODE_MOVE;
				return TRUE;
			}
			for( int axis = 0; axis < 3; axis++ ){
				int x1, y1, x2, y2;
				double dx, dy;
				if( GetAntGizmoAxisScreen(wx, wy, wz, axis, 46, x1, y1, x2, y2, dx, dy) != TRUE ) continue;
				double d = AntViewSqDistPointToSegment(X, Y, x1, y1, x2, y2);
				if( d < best ){
					best = d;
					Endpoint = 3;
					Axis = axis;
					Mode = ANT_GIZMO_MODE_MOVE;
					found = TRUE;
				}
			}
			if( HitAntRotationGizmo(wx, wy, wz, 3, X, Y, Endpoint, Axis, Mode, best) == TRUE ){
				found = TRUE;
			}
		}
		return found;
	}

	int w = Grid2->Row - 1;
	if( (w < 0) || (w >= ant.wmax) ) return FALSE;
	WDEF *wp = &ant.wdef[w];
	for( int endp = 0; endp < 3; endp++ ){
		double wx = endp ? wp->X2 : wp->X1;
		double wy = endp ? wp->Y2 : wp->Y1;
		double wz = endp ? wp->Z2 : wp->Z1;
		if( endp == 2 ){
			wx = (wp->X1 + wp->X2) / 2.0;
			wy = (wp->Y1 + wp->Y2) / 2.0;
			wz = (wp->Z1 + wp->Z2) / 2.0;
		}
		int sx, sy;
		AntWorldToScreen(wx, wy, wz, sx, sy);
		int mdx = sx - X;
		int mdy = sy - Y;
		if( ((mdx * mdx) + (mdy * mdy)) <= (7 * 7) ){
			Endpoint = endp;
			Axis = -1;
			Mode = ANT_GIZMO_MODE_MOVE;
			return TRUE;
		}
		for( int axis = 0; axis < 3; axis++ ){
			int x1, y1, x2, y2;
			double dx, dy;
			if( GetAntGizmoAxisScreen(wx, wy, wz, axis, 42, x1, y1, x2, y2, dx, dy) != TRUE ) continue;
			double d = AntViewSqDistPointToSegment(X, Y, x1, y1, x2, y2);
			if( d < best ){
				best = d;
				Endpoint = endp;
				Axis = axis;
				Mode = ANT_GIZMO_MODE_MOVE;
				found = TRUE;
			}
		}
		if( endp == 2 ){
			if( HitAntRotationGizmo(wx, wy, wz, endp, X, Y, Endpoint, Axis, Mode, best) == TRUE ){
				found = TRUE;
			}
		}
	}
	return found;
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::BeginAntGizmoDrag(int X, int Y)
{
	int endpoint, axis, mode;
	if( HitAntEditGizmo(X, Y, endpoint, axis, mode) != TRUE ) return FALSE;
	int w = Grid2->Row - 1;
	if( (w < 0) || (w >= ant.wmax) ) return FALSE;

	WDEF *wp = &ant.wdef[w];
	double wx = endpoint ? wp->X2 : wp->X1;
	double wy = endpoint ? wp->Y2 : wp->Y1;
	double wz = endpoint ? wp->Z2 : wp->Z1;
	if( endpoint == 3 ){
		GetAntSelectionCenter(wx, wy, wz);
	}
	else if( endpoint == 2 ){
		wx = (wp->X1 + wp->X2) / 2.0;
		wy = (wp->Y1 + wp->Y2) / 2.0;
		wz = (wp->Z1 + wp->Z2) / 2.0;
	}
	int x1, y1, x2, y2;
	double dx = 0.0;
	double dy = 0.0;
	double startAngle = 0.0;
	if( (mode == ANT_GIZMO_MODE_MOVE) && (axis >= 0) ){
		if( GetAntGizmoAxisScreen(wx, wy, wz, axis, 42, x1, y1, x2, y2, dx, dy) != TRUE ) return FALSE;
	}
	else if( mode == ANT_GIZMO_MODE_ROTATE ){
		if( GetAntGizmoRotationMouseAngle(X, Y, axis, wx, wy, wz, startAngle) != TRUE ) return FALSE;
	}

	AntGizmoEndpoint = endpoint;
	AntGizmoAxis = axis;
	AntGizmoMode = mode;
	AntGizmoWire = w;
	AntGizmoMouseX = X;
	AntGizmoMouseY = Y;
	AntGizmoAxisDX = dx;
	AntGizmoAxisDY = dy;
	AntGizmoStartAngle = startAngle;
	AntGizmoAngle = 0.0;
	memcpy(&AntGizmoOldW, wp, sizeof(WDEF));
	memcpy(AntGizmoOldSelected, ant.wdef, sizeof(AntGizmoOldSelected));
	AntGizmoDrag = TRUE;
	AntGizmoDragMoved = FALSE;
	PBoxAntIgnoreClick = TRUE;
	PBoxAnt->Cursor = crSizeAll;
	PBoxAnt->Invalidate();
	return TRUE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::ResetAntGizmoDrag(void)
{
	AntGizmoDrag = FALSE;
	AntGizmoDragMoved = FALSE;
	AntGizmoWire = -1;
	AntGizmoShowSnapVertices = FALSE;
	AntGizmoMode = ANT_GIZMO_MODE_MOVE;
	AntGizmoStartAngle = 0.0;
	AntGizmoAngle = 0.0;
	PBoxAnt->Cursor = AntDrawMode ? crCross : crDefault;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::ApplyAntGizmoInput(double DX, double DY, double DZ, double Angle)
{
	if( (AntGizmoWire < 0) || (AntGizmoWire >= ant.wmax) ) return FALSE;

	memcpy(ant.wdef, AntGizmoOldSelected, sizeof(WDEF) * ant.wmax);
	WDEF *wp = &ant.wdef[AntGizmoWire];
	if( AntGizmoMode == ANT_GIZMO_MODE_ROTATE ){
		if( ABS(Angle) < 1.0e-12 ) return FALSE;
		double sx, sy, sz;
		GetAntGizmoAnchor(sx, sy, sz);
		for( int i = 0; i < ant.wmax; i++ ){
			if( !IsAntWireSelected(i) ) continue;
			memcpy(&ant.wdef[i], &AntGizmoOldSelected[i], sizeof(WDEF));
			AntRotateWireAroundGizmoAxis(&ant.wdef[i], sx, sy, sz, AntGizmoAxis, Angle);
		}
		ant.Edit = ant.Flag = 1;
		return TRUE;
	}

	if( (ABS(DX) + ABS(DY) + ABS(DZ)) < 1.0e-12 ) return FALSE;

	if( AntGizmoEndpoint == 0 ){
		wp->X1 = AntGizmoOldW.X1 + DX;
		wp->Y1 = AntGizmoOldW.Y1 + DY;
		wp->Z1 = AntGizmoOldW.Z1 + DZ;
	}
	else if( AntGizmoEndpoint == 1 ){
		wp->X2 = AntGizmoOldW.X2 + DX;
		wp->Y2 = AntGizmoOldW.Y2 + DY;
		wp->Z2 = AntGizmoOldW.Z2 + DZ;
	}
	else if( AntGizmoEndpoint == 2 ){
		memcpy(wp, &AntGizmoOldW, sizeof(WDEF));
		wp->X1 += DX; wp->Y1 += DY; wp->Z1 += DZ;
		wp->X2 += DX; wp->Y2 += DY; wp->Z2 += DZ;
	}
	else if( AntGizmoEndpoint == 3 ){
		for( int i = 0; i < ant.wmax; i++ ){
			if( !IsAntWireSelected(i) ) continue;
			memcpy(&ant.wdef[i], &AntGizmoOldSelected[i], sizeof(WDEF));
			ant.wdef[i].X1 += DX; ant.wdef[i].Y1 += DY; ant.wdef[i].Z1 += DZ;
			ant.wdef[i].X2 += DX; ant.wdef[i].Y2 += DY; ant.wdef[i].Z2 += DZ;
		}
	}

	if( (ChkWith->Checked == TRUE) && (AntGizmoEndpoint != 3) ){
		if( AntGizmoEndpoint == 0 ){
			AntMoveConnectedPoint(ant.wdef, AntGizmoOldSelected, ant.wmax,
				AntGizmoOldW.X1, AntGizmoOldW.Y1, AntGizmoOldW.Z1,
				wp->X1, wp->Y1, wp->Z1);
		}
		else if( AntGizmoEndpoint == 1 ){
			AntMoveConnectedPoint(ant.wdef, AntGizmoOldSelected, ant.wmax,
				AntGizmoOldW.X2, AntGizmoOldW.Y2, AntGizmoOldW.Z2,
				wp->X2, wp->Y2, wp->Z2);
		}
		else if( AntGizmoEndpoint == 2 ){
			AntMoveConnectedWirePoints(ant.wdef, AntGizmoOldSelected, ant.wmax, AntGizmoOldW, *wp);
		}
	}

	ant.Edit = ant.Flag = 1;
	return TRUE;
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::ShowAntGizmoInput(void)
{
	double dx, dy, dz, angle;
	if( AntGizmoPromptInput(this, AntGizmoMode, AntGizmoAxis, dx, dy, dz, angle) != TRUE ){
		return FALSE;
	}
	if( (AntGizmoMode == ANT_GIZMO_MODE_ROTATE) && (ABS(angle) < 1.0e-12) ){
		return FALSE;
	}
	if( (AntGizmoMode == ANT_GIZMO_MODE_MOVE) && ((ABS(dx) + ABS(dy) + ABS(dz)) < 1.0e-12) ){
		return FALSE;
	}
	PushAntUndo();
	return ApplyAntGizmoInput(dx, dy, dz, angle);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::GetAntGizmoAnchor(double &X, double &Y, double &Z)
{
	X = AntGizmoOldW.X1;
	Y = AntGizmoOldW.Y1;
	Z = AntGizmoOldW.Z1;
	if( AntGizmoEndpoint == 1 ){
		X = AntGizmoOldW.X2;
		Y = AntGizmoOldW.Y2;
		Z = AntGizmoOldW.Z2;
	}
	else if( AntGizmoEndpoint == 2 ){
		X = (AntGizmoOldW.X1 + AntGizmoOldW.X2) / 2.0;
		Y = (AntGizmoOldW.Y1 + AntGizmoOldW.Y2) / 2.0;
		Z = (AntGizmoOldW.Z1 + AntGizmoOldW.Z2) / 2.0;
	}
	else if( AntGizmoEndpoint == 3 ){
		X = Y = Z = 0.0;
		int cnt = 0;
		for( int i = 0; i < ant.wmax; i++ ){
			if( !IsAntWireSelected(i) ) continue;
			X += (AntGizmoOldSelected[i].X1 + AntGizmoOldSelected[i].X2) / 2.0;
			Y += (AntGizmoOldSelected[i].Y1 + AntGizmoOldSelected[i].Y2) / 2.0;
			Z += (AntGizmoOldSelected[i].Z1 + AntGizmoOldSelected[i].Z2) / 2.0;
			cnt++;
		}
		if( cnt > 0 ){
			X /= double(cnt);
			Y /= double(cnt);
			Z /= double(cnt);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::GetAntGizmoFreeDelta(int X, int Y, double &DX, double &DY, double &DZ)
{
	DX = DY = DZ = 0.0;
	double sx, sy, sz;
	GetAntGizmoAnchor(sx, sy, sz);

	int x0, y0, xx, yx, xy, yy, xz, yz;
	AntWorldToScreen(sx, sy, sz, x0, y0);
	AntWorldToScreen(sx + 1.0, sy, sz, xx, yx);
	AntWorldToScreen(sx, sy + 1.0, sz, xy, yy);
	AntWorldToScreen(sx, sy, sz + 1.0, xz, yz);
	double bx0 = double(xx - x0);
	double by0 = double(yx - y0);
	double bx1 = double(xy - x0);
	double by1 = double(yy - y0);
	double bx2 = double(xz - x0);
	double by2 = double(yz - y0);
	double mx = double(X - AntGizmoMouseX);
	double my = double(Y - AntGizmoMouseY);
	double a = (bx0 * bx0) + (bx1 * bx1) + (bx2 * bx2);
	double b = (bx0 * by0) + (bx1 * by1) + (bx2 * by2);
	double c = (by0 * by0) + (by1 * by1) + (by2 * by2);
	double det = (a * c) - (b * b);
	if( ABS(det) < 1.0e-9 ) return;
	double px = ((c * mx) - (b * my)) / det;
	double py = ((a * my) - (b * mx)) / det;
	DX = (bx0 * px) + (by0 * py);
	DY = (bx1 * px) + (by1 * py);
	DZ = (bx2 * px) + (by2 * py);
}
//---------------------------------------------------------------------------
int __fastcall TMainWnd::GetAntGizmoRotationMouseAngle(int X, int Y, int Axis,
	double CX, double CY, double CZ, double &Angle)
{
	TAntViewParams view;
	view.Width = PBoxAnt->Width;
	view.Height = PBoxAnt->Height;
	view.CenterX = exeenv.AntXc;
	view.CenterY = exeenv.AntYc;
	view.Deg = TBarDeg->Position;
	view.ZDeg = TBarZDeg->Position;
	view.Scale = GetAntViewScale();
	view.UnitScale = GetAntViewUnitScale();

	TAntDrawPlaneState plane;
	plane.Plane = AntGizmoRotationPlane(Axis);
	plane.Active = TRUE;
	plane.X1 = CX;
	plane.Y1 = CY;
	plane.Z1 = CZ;

	double wx, wy, wz;
	if( AntViewScreenToPlane(view, plane, X, Y, wx, wy, wz) == TRUE ){
		if( (ABS(wx - CX) + ABS(wy - CY) + ABS(wz - CZ)) > 1.0e-9 ){
			Angle = AntGizmoWorldRotationAngle(Axis, CX, CY, CZ, wx, wy, wz);
			return TRUE;
		}
	}

	int sx, sy;
	AntWorldToScreen(CX, CY, CZ, sx, sy);
	double dx = double(X - sx);
	double dy = double(sy - Y);
	if( ((dx * dx) + (dy * dy)) < 1.0 ) return FALSE;
	Angle = atan2(dy, dx) * 180.0 / PAI;
	return TRUE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::UpdateAntGizmoDrag(int X, int Y, TShiftState Shift)
{
	if( !AntGizmoDrag ) return;
	if( (AntGizmoWire < 0) || (AntGizmoWire >= ant.wmax) ) return;
	double mx = double(X - AntGizmoMouseX);
	double my = double(Y - AntGizmoMouseY);
	bool ctrl = AntCtrlDown(Shift);
	bool shift = AntShiftDown(Shift);
	bool moveConnected = ctrl && shift;
	AntGizmoShowSnapVertices = (AntGizmoMode == ANT_GIZMO_MODE_MOVE) && shift && !ctrl;
	memcpy(ant.wdef, AntGizmoOldSelected, sizeof(WDEF) * ant.wmax);
	WDEF *wp = &ant.wdef[AntGizmoWire];
	double sx, sy, sz;
	GetAntGizmoAnchor(sx, sy, sz);
	if( AntGizmoMode == ANT_GIZMO_MODE_ROTATE ){
		double angle;
		if( GetAntGizmoRotationMouseAngle(X, Y, AntGizmoAxis, sx, sy, sz, angle) != TRUE ) return;
		AntGizmoAngle = AntGizmoNormalizeAngle(angle - AntGizmoStartAngle);
		if( ctrl ) AntGizmoAngle = AntGizmoSnapAngle(AntGizmoAngle, 15.0);
		else if( shift ) AntGizmoAngle = AntGizmoSnapAngle(AntGizmoAngle, 5.0);
		for( int i = 0; i < ant.wmax; i++ ){
			if( !IsAntWireSelected(i) ) continue;
			memcpy(&ant.wdef[i], &AntGizmoOldSelected[i], sizeof(WDEF));
			AntRotateWireAroundGizmoAxis(&ant.wdef[i], sx, sy, sz, AntGizmoAxis, AntGizmoAngle);
		}
		ant.Edit = ant.Flag = 1;
		Grid2->Invalidate();
		PBoxAnt->Invalidate();
		return;
	}
	if( AntGizmoAxis < 0 ){
		double dx, dy, dz;
		GetAntGizmoFreeDelta(X, Y, dx, dy, dz);
		int rx, ry;
		AntWorldToScreen(sx + dx, sy + dy, sz + dz, rx, ry);
		double tx, ty, tz;
		if( shift && !ctrl && (FindAntSnapVertex(rx, ry, tx, ty, tz) == TRUE) ){
			dx = tx - sx;
			dy = ty - sy;
			dz = tz - sz;
		}
		else if( ctrl && !shift && (FindAntSnapEdge(rx, ry, tx, ty, tz) == TRUE) ){
			dx = tx - sx;
			dy = ty - sy;
			dz = tz - sz;
		}
		if( AntGizmoEndpoint == 0 ){
			wp->X1 = AntGizmoOldW.X1 + dx;
			wp->Y1 = AntGizmoOldW.Y1 + dy;
			wp->Z1 = AntGizmoOldW.Z1 + dz;
		}
		else if( AntGizmoEndpoint == 1 ){
			wp->X2 = AntGizmoOldW.X2 + dx;
			wp->Y2 = AntGizmoOldW.Y2 + dy;
			wp->Z2 = AntGizmoOldW.Z2 + dz;
		}
		else if( AntGizmoEndpoint == 2 ){
			memcpy(wp, &AntGizmoOldW, sizeof(WDEF));
			wp->X1 += dx; wp->Y1 += dy; wp->Z1 += dz;
			wp->X2 += dx; wp->Y2 += dy; wp->Z2 += dz;
		}
		else if( AntGizmoEndpoint == 3 ){
			for( int i = 0; i < ant.wmax; i++ ){
				if( !IsAntWireSelected(i) ) continue;
				memcpy(&ant.wdef[i], &AntGizmoOldSelected[i], sizeof(WDEF));
				ant.wdef[i].X1 += dx; ant.wdef[i].Y1 += dy; ant.wdef[i].Z1 += dz;
				ant.wdef[i].X2 += dx; ant.wdef[i].Y2 += dy; ant.wdef[i].Z2 += dz;
			}
		}
		if( moveConnected ){
			if( AntGizmoEndpoint == 0 ){
				AntMoveConnectedPoint(ant.wdef, AntGizmoOldSelected, ant.wmax,
					AntGizmoOldW.X1, AntGizmoOldW.Y1, AntGizmoOldW.Z1,
					wp->X1, wp->Y1, wp->Z1);
			}
			else if( AntGizmoEndpoint == 1 ){
				AntMoveConnectedPoint(ant.wdef, AntGizmoOldSelected, ant.wmax,
					AntGizmoOldW.X2, AntGizmoOldW.Y2, AntGizmoOldW.Z2,
					wp->X2, wp->Y2, wp->Z2);
			}
			else if( AntGizmoEndpoint == 2 ){
				AntMoveConnectedWirePoints(ant.wdef, AntGizmoOldSelected, ant.wmax, AntGizmoOldW, *wp);
			}
			else if( AntGizmoEndpoint == 3 ){
				for( int i = 0; i < ant.wmax; i++ ){
					if( IsAntWireSelected(i) ){
						AntMoveConnectedWirePoints(ant.wdef, AntGizmoOldSelected, ant.wmax,
							AntGizmoOldSelected[i], ant.wdef[i]);
					}
				}
			}
		}
		ant.Edit = ant.Flag = 1;
		Grid2->Invalidate();
		PBoxAnt->Invalidate();
		return;
	}
	double len2 = (AntGizmoAxisDX * AntGizmoAxisDX) + (AntGizmoAxisDY * AntGizmoAxisDY);
	if( len2 <= 0.0 ) return;

	double d = ((mx * AntGizmoAxisDX) + (my * AntGizmoAxisDY)) / len2;
	double ax = (AntGizmoAxis == ANT_GIZMO_AXIS_X) ? 1.0 : 0.0;
	double ay = (AntGizmoAxis == ANT_GIZMO_AXIS_Y) ? 1.0 : 0.0;
	double az = (AntGizmoAxis == ANT_GIZMO_AXIS_Z) ? 1.0 : 0.0;
	int rx, ry;
	AntWorldToScreen(sx + (ax * d), sy + (ay * d), sz + (az * d), rx, ry);
	double tx, ty, tz;
	if( shift && !ctrl && (FindAntSnapVertex(rx, ry, tx, ty, tz) == TRUE) ){
		switch( AntGizmoAxis ){
			case ANT_GIZMO_AXIS_X: d = tx - sx; break;
			case ANT_GIZMO_AXIS_Y: d = ty - sy; break;
			case ANT_GIZMO_AXIS_Z: d = tz - sz; break;
		}
	}
	else if( ctrl && !shift && (FindAntSnapEdge(rx, ry, tx, ty, tz) == TRUE) ){
		switch( AntGizmoAxis ){
			case ANT_GIZMO_AXIS_X: d = tx - sx; break;
			case ANT_GIZMO_AXIS_Y: d = ty - sy; break;
			case ANT_GIZMO_AXIS_Z: d = tz - sz; break;
		}
	}
	if( AntGizmoEndpoint == 0 ){
		wp->X1 = AntGizmoOldW.X1;
		wp->Y1 = AntGizmoOldW.Y1;
		wp->Z1 = AntGizmoOldW.Z1;
		switch( AntGizmoAxis ){
			case ANT_GIZMO_AXIS_X: wp->X1 += d; break;
			case ANT_GIZMO_AXIS_Y: wp->Y1 += d; break;
			case ANT_GIZMO_AXIS_Z: wp->Z1 += d; break;
		}
	}
	else if( AntGizmoEndpoint == 1 ){
		wp->X2 = AntGizmoOldW.X2;
		wp->Y2 = AntGizmoOldW.Y2;
		wp->Z2 = AntGizmoOldW.Z2;
		switch( AntGizmoAxis ){
			case ANT_GIZMO_AXIS_X: wp->X2 += d; break;
			case ANT_GIZMO_AXIS_Y: wp->Y2 += d; break;
			case ANT_GIZMO_AXIS_Z: wp->Z2 += d; break;
		}
	}
	else if( AntGizmoEndpoint == 2 ){
		memcpy(wp, &AntGizmoOldW, sizeof(WDEF));
		switch( AntGizmoAxis ){
			case ANT_GIZMO_AXIS_X: wp->X1 += d; wp->X2 += d; break;
			case ANT_GIZMO_AXIS_Y: wp->Y1 += d; wp->Y2 += d; break;
			case ANT_GIZMO_AXIS_Z: wp->Z1 += d; wp->Z2 += d; break;
		}
	}
	else {
		for( int i = 0; i < ant.wmax; i++ ){
			if( !IsAntWireSelected(i) ) continue;
			memcpy(&ant.wdef[i], &AntGizmoOldSelected[i], sizeof(WDEF));
			switch( AntGizmoAxis ){
				case ANT_GIZMO_AXIS_X: ant.wdef[i].X1 += d; ant.wdef[i].X2 += d; break;
				case ANT_GIZMO_AXIS_Y: ant.wdef[i].Y1 += d; ant.wdef[i].Y2 += d; break;
				case ANT_GIZMO_AXIS_Z: ant.wdef[i].Z1 += d; ant.wdef[i].Z2 += d; break;
			}
		}
	}
	if( moveConnected ){
		if( AntGizmoEndpoint == 0 ){
			AntMoveConnectedPoint(ant.wdef, AntGizmoOldSelected, ant.wmax,
				AntGizmoOldW.X1, AntGizmoOldW.Y1, AntGizmoOldW.Z1,
				wp->X1, wp->Y1, wp->Z1);
		}
		else if( AntGizmoEndpoint == 1 ){
			AntMoveConnectedPoint(ant.wdef, AntGizmoOldSelected, ant.wmax,
				AntGizmoOldW.X2, AntGizmoOldW.Y2, AntGizmoOldW.Z2,
				wp->X2, wp->Y2, wp->Z2);
		}
		else if( AntGizmoEndpoint == 2 ){
			AntMoveConnectedWirePoints(ant.wdef, AntGizmoOldSelected, ant.wmax, AntGizmoOldW, *wp);
		}
		else if( AntGizmoEndpoint == 3 ){
			for( int i = 0; i < ant.wmax; i++ ){
				if( IsAntWireSelected(i) ){
					AntMoveConnectedWirePoints(ant.wdef, AntGizmoOldSelected, ant.wmax,
						AntGizmoOldSelected[i], ant.wdef[i]);
				}
			}
		}
	}
	ant.Edit = ant.Flag = 1;
	Grid2->Invalidate();
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::EndAntGizmoDrag(void)
{
	if( !AntGizmoDrag ) return;
	int w = AntGizmoWire;
	AntGizmoDrag = FALSE;
	AntGizmoDragMoved = FALSE;
	AntGizmoWire = -1;
	AntGizmoShowSnapVertices = FALSE;
	AntGizmoMode = ANT_GIZMO_MODE_MOVE;
	AntGizmoStartAngle = 0.0;
	AntGizmoAngle = 0.0;
	PBoxAnt->Cursor = AntDrawMode ? crCross : crDefault;
	if( (w >= 0) && (w < ant.wmax) ){
		if( AntGizmoEndpoint == 3 ){
			UpdateAntData();
			RequestAutoCalc(TRUE);
			return;
		}
		if( ChkWith->Checked == TRUE ){
			AdjWireChen(ant.wdef, ant.wmax, &ant.wdef[w], &AntGizmoOldW);
		}
		UpdateAntData();
		RequestAutoCalc(TRUE);
	}
	else {
		PBoxAnt->Invalidate();
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntGizmoMoveDistance(void)
{
	if( !AntGizmoDrag ) return;
	if( (AntGizmoWire < 0) || (AntGizmoWire >= ant.wmax) ) return;

	double ox, oy, oz;
	if( AntGizmoMode == ANT_GIZMO_MODE_ROTATE ){
		GetAntGizmoAnchor(ox, oy, oz);
		int sx, sy;
		AntWorldToScreen(ox, oy, oz, sx, sy);
		char bf[160];
		sprintf(bf, "Rotate:%.1f deg", AntGizmoAngle);
		PaintAntLengthLabel(AntPaintCanvas(), PBoxAnt->Width, PBoxAnt->Height, sx, sy, bf);
		return;
	}

	double cx, cy, cz;
	GetAntGizmoAnchor(ox, oy, oz);
	WDEF *wp = &ant.wdef[AntGizmoWire];
	if( AntGizmoEndpoint == 0 ){
		cx = wp->X1;
		cy = wp->Y1;
		cz = wp->Z1;
	}
	else if( AntGizmoEndpoint == 1 ){
		cx = wp->X2;
		cy = wp->Y2;
		cz = wp->Z2;
	}
	else if( AntGizmoEndpoint == 2 ){
		cx = (wp->X1 + wp->X2) / 2.0;
		cy = (wp->Y1 + wp->Y2) / 2.0;
		cz = (wp->Z1 + wp->Z2) / 2.0;
	}
	else if( GetAntSelectionCenter(cx, cy, cz) != TRUE ){
		return;
	}

	double dx = cx - ox;
	double dy = cy - oy;
	double dz = cz - oz;
	double dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));
	int sx, sy;
	AntWorldToScreen(cx, cy, cz, sx, sy);

	char bf[160];
	FormatAntLengthText(bf, "Move", dist);
	PaintAntLengthLabel(AntPaintCanvas(), PBoxAnt->Width, PBoxAnt->Height, sx, sy, bf);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PaintAntDrawPreview(void)
{
	if( !AntDrawMode || !AntDrawActive ) return;
	int X1, Y1, X2, Y2;
	AntWorldToScreen(AntDrawX1, AntDrawY1, AntDrawZ1, X1, Y1);
	AntWorldToScreen(AntDrawX2, AntDrawY2, AntDrawZ2, X2, Y2);

	TCanvas *Canvas = AntPaintCanvas();
	TColor oldColor = Canvas->Pen->Color;
	TPenStyle oldStyle = Canvas->Pen->Style;
	int oldWidth = Canvas->Pen->Width;
	Canvas->Pen->Color = clRed;
	Canvas->Pen->Style = psDash;
	Canvas->Pen->Width = 1;
	Canvas->MoveTo(X1, Y1);
	Canvas->LineTo(X2, Y2);
	Canvas->Pen->Style = psSolid;
	Canvas->Ellipse(X1-3, Y1-3, X1+4, Y1+4);
	Canvas->Ellipse(X2-3, Y2-3, X2+4, Y2+4);
	Canvas->Pen->Color = oldColor;
	Canvas->Pen->Style = oldStyle;
	Canvas->Pen->Width = oldWidth;

	double dx = AntDrawX2 - AntDrawX1;
	double dy = AntDrawY2 - AntDrawY1;
	double dz = AntDrawZ2 - AntDrawZ1;
	double dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));
	char bf[160];
	FormatAntLengthText(bf, "Length", dist);
	PaintAntLengthLabel(Canvas, PBoxAnt->Width, PBoxAnt->Height, (X1 + X2) / 2, (Y1 + Y2) / 2, bf);
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AddAntDrawWire(double X1, double Y1, double Z1, double X2, double Y2, double Z2)
{
	PushAntUndo();
	if( ant.wmax >= WMAX ){
		::MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	if( (ABS(X2-X1) + ABS(Y2-Y1) + ABS(Z2-Z1)) < 1.0e-9 ) return;

	int n = ant.wmax;
	WDEF *wp = &ant.wdef[n];
	memset(wp, 0, sizeof(WDEF));
	wp->X1 = X1;
	wp->Y1 = Y1;
	wp->Z1 = Z1;
	wp->X2 = X2;
	wp->Y2 = Y2;
	wp->Z2 = Z2;
	wp->R = 0.0008;
	wp->SEG = 0;
	if( n ){
		if( ant.wdef[n-1].R > 0.0 ) wp->R = ant.wdef[n-1].R;
		wp->SEG = ant.wdef[n-1].SEG;
	}
	ant.wmax++;
	Grid2->RowCount = ant.wmax + 2;
	Grid2->Row = n + 1;
	UpdateAntData();
	if( n < (Grid2->RowCount - 1) ) Grid2->Row = n + 1;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::AntViewZoom(int Delta, int X, int Y)
{
	double oldSc = GetAntViewScale();
	int oldPos = TBarSC->Position;
	AntViewTrackBarChanging = TRUE;
	SetTrackBarPosition(TBarSC, TBarSC->Position + Delta);
	AntViewTrackBarChanging = FALSE;
	if( oldPos == TBarSC->Position ) return;
	double newSc = GetAntViewScale();
	if( (oldSc > 0.0) && (newSc > 0.0) ){
		exeenv.AntXc += double(X - (PBoxAnt->Width / 2)) * ((1.0 / newSc) - (1.0 / oldSc));
		exeenv.AntYc += double(Y - (PBoxAnt->Height / 2)) * ((1.0 / newSc) - (1.0 / oldSc));
	}
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
// アンテナ形状表示のマウスイベント
void __fastcall TMainWnd::PBoxAntMouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y)
{
	if( CalcBusyDepth ) return;
	PBoxAntMX = X;
	PBoxAntMY = Y;
	PBoxAntClickCtrl = Shift.Contains(ssCtrl) ? TRUE : FALSE;
	PBoxAntClickShift = Shift.Contains(ssShift) ? TRUE : FALSE;
	if( (Button == mbLeft) && BeginAntGizmoDrag(X, Y) == TRUE ){
		PBoxAntIgnoreClick = TRUE;
		PBoxAntDragButton = -1;
		PBoxAntDragAction = ANT_MOUSE_NONE;
		return;
	}
	if( AntDrawMode ){
		PBoxAntIgnoreClick = TRUE;
		PBoxAntDragButton = -1;
		PBoxAntDragAction = ANT_MOUSE_NONE;
		if( Button == mbRight ){
			AntDrawActive = FALSE;
			AntDrawSnapVisible = FALSE;
			PBoxAnt->Invalidate();
			return;
		}
		if( Button != mbLeft ) return;
		double wx, wy, wz;
		if( AntViewPointToXY(X, Y, wx, wy, wz) != TRUE ){
			::MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
		SnapAntDrawPoint(X, Y, wx, wy, wz);
		if( !AntDrawActive ){
			AntDrawX1 = AntDrawX2 = wx;
			AntDrawY1 = AntDrawY2 = wy;
			AntDrawZ1 = AntDrawZ2 = wz;
			AntDrawActive = TRUE;
			PBoxAnt->Invalidate();
		}
		else {
			AntDrawX2 = wx;
			AntDrawY2 = wy;
			AntDrawZ2 = wz;
			AddAntDrawWire(AntDrawX1, AntDrawY1, AntDrawZ1, AntDrawX2, AntDrawY2, AntDrawZ2);
			AntDrawActive = FALSE;
		}
		return;
	}
	if( (Button == mbLeft) && Shift.Contains(ssShift) ){
		PBoxAntSelectRect = TRUE;
		PBoxAntRectX = X;
		PBoxAntRectY = Y;
		PBoxAntLastX = X;
		PBoxAntLastY = Y;
		PBoxAntDragButton = int(Button);
		PBoxAntDragAction = ANT_MOUSE_NONE;
		PBoxAntDragMoved = FALSE;
		return;
	}
	PBoxAntLastX = X;
	PBoxAntLastY = Y;
	PBoxAntStartDeg = TBarDeg->Position;
	PBoxAntStartZDeg = TBarZDeg->Position;
	PBoxAntDragButton = int(Button);
	PBoxAntDragAction = GetAntMouseAction(Button);
	PBoxAntDragMoved = FALSE;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PBoxAntMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	if( CalcBusyDepth ) return;
	if( AntGizmoDrag ){
		if( !AntGizmoDragMoved && (ABS(X - AntGizmoMouseX) < 3) && (ABS(Y - AntGizmoMouseY) < 3) ) return;
		if( !AntGizmoDragMoved ){
			PushAntUndo();
			AntGizmoDragMoved = TRUE;
		}
		UpdateAntGizmoDrag(X, Y, Shift);
		return;
	}
	if( AntDrawMode ){
		double wx, wy, wz;
		if( AntViewPointToXY(X, Y, wx, wy, wz) == TRUE ){
			SnapAntDrawPoint(X, Y, wx, wy, wz);
			if( AntDrawActive ){
				AntDrawX2 = wx;
				AntDrawY2 = wy;
				AntDrawZ2 = wz;
			}
		}
		else {
			AntDrawSnapVisible = FALSE;
		}
		PBoxAnt->Invalidate();
		return;
	}
	if( PBoxAntSelectRect ){
		PBoxAntRectX = X;
		PBoxAntRectY = Y;
		if( !PBoxAntDragMoved && (ABS(X - PBoxAntMX) < 3) && (ABS(Y - PBoxAntMY) < 3) ) return;
		PBoxAntDragMoved = TRUE;
		PBoxAnt->Invalidate();
		return;
	}
	if( PBoxAntDragButton < 0 ) return;
	int dx = X - PBoxAntLastX;
	int dy = Y - PBoxAntLastY;
	if( !dx && !dy ) return;
	if( !PBoxAntDragMoved && (ABS(X - PBoxAntMX) < 3) && (ABS(Y - PBoxAntMY) < 3) ) return;
	PBoxAntDragMoved = TRUE;
	if( PBoxAntDragAction == ANT_MOUSE_NONE ) return;
	switch( PBoxAntDragAction ){
		case ANT_MOUSE_PAN:
			AntViewPan(dx, dy);
			break;
		case ANT_MOUSE_ROTATE:
			AntViewRotateDrag(X, Y);
			break;
		case ANT_MOUSE_ZOOM:
			AntViewZoom(-dy / 2, X, Y);
			break;
	}
	PBoxAntLastX = X;
	PBoxAntLastY = Y;
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PBoxAntMouseUp(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y)
{
	if( CalcBusyDepth ) return;
	if( AntGizmoDrag ){
		int moved = AntGizmoDragMoved || (ABS(X - AntGizmoMouseX) >= 3) || (ABS(Y - AntGizmoMouseY) >= 3);
		if( moved ){
			if( !AntGizmoDragMoved ){
				PushAntUndo();
				AntGizmoDragMoved = TRUE;
			}
			UpdateAntGizmoDrag(X, Y, Shift);
			EndAntGizmoDrag();
		}
		else {
			int applied = ShowAntGizmoInput();
			ResetAntGizmoDrag();
			if( applied ){
				UpdateAntData();
				RequestAutoCalc(TRUE);
			}
		}
		PBoxAntIgnoreClick = TRUE;
		return;
	}
	if( AntDrawMode ){
		PBoxAntDragButton = -1;
		PBoxAntDragAction = ANT_MOUSE_NONE;
		return;
	}
	if( PBoxAntSelectRect ){
		int moved = PBoxAntDragMoved || (ABS(X - PBoxAntMX) >= 3) || (ABS(Y - PBoxAntMY) >= 3);
		PBoxAntRectX = X;
		PBoxAntRectY = Y;
		PBoxAntSelectRect = FALSE;
		PBoxAntDragButton = -1;
		PBoxAntDragAction = ANT_MOUSE_NONE;
		PBoxAntDragMoved = FALSE;
		if( moved ){
			SelectWiresInRect(PBoxAntMX, PBoxAntMY, X, Y);
			PBoxAntIgnoreClick = TRUE;
		}
		else if( Button == mbLeft ){
			SelectAntWireClick(PBoxAntMX, PBoxAntMY);
			PBoxAntIgnoreClick = TRUE;
		}
		PBoxAnt->Invalidate();
		return;
	}
	if( int(Button) == PBoxAntDragButton ){
		if( (ABS(X - PBoxAntMX) >= 3) || (ABS(Y - PBoxAntMY) >= 3) ) PBoxAntDragMoved = TRUE;
		int moved = PBoxAntDragMoved;
		PBoxAntDragButton = -1;
		PBoxAntDragAction = ANT_MOUSE_NONE;
		PBoxAntDragMoved = FALSE;
		if( moved ){
			PBoxAntIgnoreClick = TRUE;
		}
		else if( Button == mbLeft ){
			SelectAntWireClick(PBoxAntMX, PBoxAntMY);
			PBoxAntIgnoreClick = TRUE;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::PBoxAntMouseWheel(TObject *Sender, TShiftState Shift,
	int WheelDelta, const TPoint &MousePos, bool &Handled)
{
	if( CalcBusyDepth ){
		Handled = TRUE;
		return;
	}
	TPoint pt = PBoxAnt->ScreenToClient(MousePos);
	if( (pt.x < 0) || (pt.y < 0) || (pt.x >= PBoxAnt->Width) || (pt.y >= PBoxAnt->Height) ) return;
	Handled = TRUE;
	int step = WheelDelta / WHEEL_DELTA;
	if( !step ) step = (WheelDelta > 0) ? 1 : -1;
	switch( exeenv.AntMouseWheel ){
		case ANT_MOUSE_PAN:
			AntViewPan(0, step * 24);
			break;
		case ANT_MOUSE_ROTATE:
			AntViewRotate(step * 5, 0);
			break;
		case ANT_MOUSE_ZOOM:
			AntViewZoom(step * 2, pt.x, pt.y);
			break;
	}
}
//---------------------------------------------------------------------------
// アンテナ形状表示のシングルクリック
void __fastcall TMainWnd::PBoxAntClick(TObject *Sender)
{
	if( PBoxAntIgnoreClick ){
		PBoxAntIgnoreClick = FALSE;
		return;
	}
	SelectAntWireClick(PBoxAntMX, PBoxAntMY);
}
//---------------------------------------------------------------------------
// アンテナ形状表示のダブルクリック
void __fastcall TMainWnd::PBoxAntDblClick(TObject *Sender)
{
	if( AntDrawMode ) return;
	int w = Grid2->Row - 1;
	if( !exeenv.CalcF && (GetAntSelectionCount() > 0) && (w >= 0) && (w < ant.wmax) && (KT1->Enabled == TRUE) ){
		KT1Click(NULL);
	}
}
//---------------------------------------------------------------------------
// ポップアップ－クリック位置を中心にする
void __fastcall TMainWnd::K29Click(TObject *Sender)
{
	double sc = double(TBarSC->Position) / 20.0;
	sc = sc * sc * sc * sc;
	exeenv.AntXc += double((PBoxAnt->Width/2) - PBoxAntMX)/sc;
	exeenv.AntYc += double((PBoxAnt->Height/2) - PBoxAntMY)/sc;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
// アンテナ形状表示の原点中心ボタン
void __fastcall TMainWnd::OrgBtnClick(TObject *Sender)
{
	exeenv.AntXc = exeenv.AntYc = 0.0;
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
// アンテナ形状表示 全景ボタン
void __fastcall TMainWnd::AllViewBtnClick(TObject *Sender)
{
	double	XL, XH, YL, YH, D, x, y;
	int		i, pos;

	if( !ant.wmax ) return;
	double deg = double(TBarDeg->Position);
	deg *= (PAI / 180.0);
	double zdeg = double(TBarZDeg->Position);
	zdeg *= (PAI / 180.0);
	XL = MAXDOUBLE; XH = -MAXDOUBLE;
	YL = MAXDOUBLE; YH = -MAXDOUBLE;
	pos = 0;
	for( i = 0; i < pCalAnt->wmax; i++ ){
		CalcAntViewXY(x, y, deg, zdeg, pCalAnt->wdef[i].X1, pCalAnt->wdef[i].Y1, pCalAnt->wdef[i].Z1);
		if( XL > x ) XL = x;
		if( XH < x ) XH = x;
		if( YL > y ) YL = y;
		if( YH < y ) YH = y;
		CalcAntViewXY(x, y, deg, zdeg, pCalAnt->wdef[i].X2, pCalAnt->wdef[i].Y2, pCalAnt->wdef[i].Z2);
		if( XL > x ) XL = x;
		if( XH < x ) XH = x;
		if( YL > y ) YL = y;
		if( YH < y ) YH = y;
		pos = 1;
	}
	if( pos ){
		exeenv.AntXc = -(XL + XH)/2;
		exeenv.AntYc = (YL + YH)/2;
		D = ABS(XH - XL);
		if( D < ABS(YH - YL) ) D = ABS(YH - YL);
		D *= 1.2;
		if( D > 0.0 ){
			D = (PBoxAnt->Height)/D;
			pos = int(sqrt(sqrt(D)) * 20.0);
			if( pos > TBarSC->Max ){
				pos = TBarSC->Max;
			}
		}
		else {
			pos = TBarSC->Max;
		}
	}
	else {
		exeenv.AntXc = exeenv.AntYc = 0;
		pos = 50;
	}
	TBarSC->Position = pos;
//	AntSC = double(TBarSC->Position) / 20.0;
//	AntSC = AntSC * AntSC * AntSC * AntSC;
//	ScrXc = int(tp->Width/2 + (AntXc * AntSC));
//	ScrYc = int(tp->Height/2 + (AntYc * AntSC));
#if 0
	D = ABS(XL);
	if( D < ABS(XH) ) D = ABS(XH);
	if( D < ABS(YL) ) D = ABS(YL);
	if( D < ABS(YH) ) D = ABS(YH);
	D *= 1.1;
	if( D > 0.0 ){
		D = (PBoxAnt->Height/2) / D;
		pos = int(sqrt(sqrt(D)) * 20.0);
		if( pos > TBarSC->Max ){
			pos = TBarSC->Max;
		}
	}
	else {
		pos = TBarSC->Max;
	}
	TBarSC->Position = pos;
	exeenv.AntXc = exeenv.AntYc = 0.0;
#endif
	PBoxAnt->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TMainWnd::TBarDegChange(TObject *Sender)
{
	if( AntViewTrackBarChanging ) return;
	PBoxAnt->Invalidate();
}
