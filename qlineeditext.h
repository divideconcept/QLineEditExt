#ifndef QLINEEDITEXT_H
#define QLINEEDITEXT_H

#include <QtCore>
#include <QtWidgets>

class QLineEditValidator : public QValidator
{
public:
    QLineEditValidator(QObject * parent = 0) : QValidator(parent)
    {
        min=max=0.;
        minStrict=maxStrict=listStrict=true;
        decimals=0;
    }
    void setRange(double min, double max, int decimals, bool minStrict, bool maxStrict) {this->min=min; this->max=max; this->decimals=decimals; this->minStrict=minStrict; this->maxStrict=maxStrict;}
    void setList(QList<double> list=QList<double>(), bool strict=true) {this->list=list; this->listStrict=strict;}
    QValidator::State validate(QString &input, int &pos) const
    {
        if(min<max && minStrict && input.toDouble()<min) return QValidator::Intermediate;
        if(min<max && maxStrict && input.toDouble()>max) return QValidator::Intermediate;
        if(!list.isEmpty() && listStrict && !list.contains(input.toDouble())) return QValidator::Invalid;
        return QValidator::Acceptable;
    }

    void fixup(QString &input) const
    {
        if(min<max && minStrict) input=QString::number(qMax(min,input.toDouble()),'f',decimals);
        if(min<max && maxStrict) input=QString::number(qMin(max,input.toDouble()),'f',decimals);
        if(!list.isEmpty() && listStrict)
        {
            for(int i=0; i<=list.count(); i++)
            {
                if(input.toDouble()<list[i] && i<list.count()) {input=QString::number(list[qMax(i-1,0)],'f',decimals); break;}
                else if(i==list.count()) input=QString::number(qMin(max,list.last()),'f',decimals);
            }
        }
    }
private:
    double min, max;
    bool minStrict, maxStrict;
    int decimals;

    QList<double> list;
    bool listStrict;
};

class QLineEditExt : public QLineEdit
{
public:
    enum IncrementDiff
    {
        Decrement=-1,
        None=0,
        Increment=1,
    };

    QLineEditExt(QWidget * parent = 0) : QLineEdit(parent), validator(this) {
        min=max=m_step=ctrlStep=0.;
        minStrict=maxStrict=listStrict=true;
        m_count=ctrlCount=0;
        decimals=0;
        defaultValue=qQNaN();
        setIncrementDragDistance();
        wheelSum=0;
        usingTouchKeyboard=false;
        progress=false;
        prefixMargin=suffixMargin=0;
        m_incrementDiff=None;
        incrementable=false;

        setAttribute(Qt::WA_AcceptTouchEvents,true);
        setMouseTracking(true);
        setValidator(&validator);
        connect(this,&QLineEditExt::editingFinished,[=](){finishEditing();});
    }
    QLineEditExt(const QString & contents, QWidget * parent = 0) : QLineEdit(contents, parent), validator(this) {
        min=max=m_step=ctrlStep=0.;
        minStrict=maxStrict=listStrict=true;
        m_count=ctrlCount=0;
        decimals=0;
        defaultValue=qQNaN();
        setIncrementDragDistance();
        wheelSum=0;
        usingTouchKeyboard=false;
        progress=false;
        prefixMargin=suffixMargin=0;
        m_incrementDiff=None;
        incrementable=false;

        setAttribute(Qt::WA_AcceptTouchEvents,true);
        setMouseTracking(true);
        setValidator(&validator);
        connect(this,&QLineEditExt::editingFinished,[=](){finishEditing();});
    }

    //Min/Max Range - also defines the number of decimals, and if set to strict, won't allow the user to go below or above the defined range
    void setRange(double min, double max, int decimals=0, bool minStrict=true, bool maxStrict=true) {this->min=min; this->max=max; this->decimals=decimals; this->minStrict=minStrict; this->maxStrict=maxStrict; validator.setRange(min,max,decimals,minStrict,maxStrict); finishEditing();}

    //Increment (either by mouse/finger drag, mouse wheel, or arrows up/down)
    //4 methods (with optionnal micro-increment when holding the CTRL key):
    //-setIncrementable (manage your own increment function, after editingFinished signal received check incrementDiff and isCtrl
    //-setStep (fixed increment)
    //-setCount (set a number of total step, and define a gamma so that increment react accordingly to micro and large scales - requires a range)
    //-setList (set specific values - if set to strict, will only accept the values from the list)
    //the setIncrementDragDistance define the distance treshold at which an increment is considered
    void setIncrementable(bool incrementable=true){this->incrementable=incrementable; if(!incrementable) {m_step=ctrlStep=0.; m_count=ctrlCount=0; m_list.clear(); validator.setList();} finishEditing();}
    void setStep(double step, double ctrlStep=0.) {this->m_step=step; this->ctrlStep=ctrlStep==0.?step:ctrlStep; m_count=ctrlCount=0; m_list.clear(); validator.setList(); incrementable=true; finishEditing();}
    void setCount(int count, int ctrlCount=0, double power=2.) {this->m_count=count; this->ctrlCount=ctrlCount==0?count:ctrlCount; this->m_power=power; m_step=ctrlStep=0.; m_list.clear(); validator.setList(); incrementable=true; finishEditing();}
    void setList(QList<double> list, bool strict=true) {this->m_list=list; this->listStrict=strict; m_step=ctrlStep=0.; m_count=ctrlCount=0; validator.setList(list,strict); incrementable=true; finishEditing();}
    void setIncrementDragDistance(int mouse=8, int touch=12) {dragStep=mouseDragStep=mouse; touchDragStep=touch;}
    bool ctrlPressed(){return qApp->queryKeyboardModifiers()&Qt::ControlModifier;}
    IncrementDiff incrementDiff(){return m_incrementDiff;}

    //Current Value - when defining a default value, user can reset to it by double-clicking the widget
    void setDefaultValue(double value) {defaultValue=value; setValue(value);}
    void setValue(double value) {setText(QString::number(value,'f',decimals)); finishEditing();}
    double value() { return text().toDouble();}
    int intValue() { return qRound(value());}

    //Decoration - show a progress bar, define optional prefix and suffix or all at once and more with the setDescription method
    //the progress bar color can be styled with the alternate-background-color stylesheet property
    void showProgress(bool show) { progress=show; update(); }
    void setPrefix(QString prefix){this->prefix=prefix+" "; update();}
    void setSuffix(QString suffix){this->suffix=" "+suffix; update();}
    void setDescription(QString prefix, QString suffix, bool progress=true, int horizontalMargin=0, Qt::Alignment alignment=Qt::AlignCenter){setContentsMargins(horizontalMargin,0,horizontalMargin,0); setAlignment(alignment); this->progress=progress; setPrefix(prefix); setSuffix(suffix);}

    //Manual increment/decrement
    void increment()
    {
        if(m_step!=0.)
            setValue(value()+currentStep());
        if(m_count!=0 && min<max)
        {
            double normalizedValue=pow((value()-min)/(max-min),1./m_power);
            normalizedValue=qMax(normalizedValue+1./double(currentCount()),0.);
            double newValue=pow(normalizedValue,m_power)*(max-min)+min;
            if(newValue-value()<pow(10.,double(-decimals))) newValue=value()+pow(10.,double(-decimals));
            setValue(newValue);
        }
        if(!m_list.isEmpty())
            setValue(m_list[qMin(m_list.indexOf(value())+1,m_list.count()-1)]);
        if(!isReadOnly())
            selectAll();
        finishEditing();
        m_incrementDiff=Increment;
        emit editingFinished();
        m_incrementDiff=None;
    }
    void decrement()
    {
        if(m_step!=0.)
            setValue(value()-currentStep());
        if(m_count!=0 && min<max)
        {
            double normalizedValue=pow((value()-min)/(max-min),1./m_power);
            normalizedValue=qMax(normalizedValue-1./double(currentCount()),0.);
            double newValue=pow(normalizedValue,m_power)*(max-min)+min;
            if(value()-newValue<pow(10.,double(-decimals))) newValue=value()-pow(10.,double(-decimals));
            setValue(newValue);
        }
        if(!m_list.isEmpty())
            setValue(m_list[qMax(m_list.indexOf(value())-1,0)]);
        if(!isReadOnly())
            selectAll();
        finishEditing();
        m_incrementDiff=Decrement;
        emit editingFinished();
        m_incrementDiff=None;
    }

protected:
    double currentStep() {return ctrlPressed()?ctrlStep:m_step;}
    int currentCount() {return ctrlPressed()?ctrlCount:m_count;}

    void finishEditing() {
        deselect();
#if defined(Q_OS_WIN32)
        if(usingTouchKeyboard)
        {
            //from http://sysmagazine.com/posts/163333/
            HWND kbd = ::FindWindow(TEXT("IPTip_Main_Window"), NULL);
            if(kbd != NULL) PostMessage(kbd, WM_SYSCOMMAND, SC_CLOSE, 0);
            usingTouchKeyboard=false;
        }
#endif
        QString input=text(); int pos=0;
        if(validator.validate(input,pos)!=QValidator::Acceptable)
        {
            validator.fixup(input);
            setText(input);
        }
        if(incrementable) {
            setReadOnly(true);
            setCursor(Qt::SizeHorCursor);
        }
    }

    void focusInEvent(QFocusEvent * event)
    {
        if(event->reason()!=Qt::MouseFocusReason)
        {
            setReadOnly(false);
            selectAll();
        }
        QLineEdit::focusInEvent(event);
    }

    void keyPressEvent(QKeyEvent * event)
    {
        if(event->key()==Qt::Key_Up) increment();
        if(event->key()==Qt::Key_Down) decrement();
        QLineEdit::keyPressEvent(event);
    }
    void wheelEvent(QWheelEvent * event)
    {
        wheelSum+=event->angleDelta().y()-event->angleDelta().x();
        if(wheelSum>=60) {increment(); wheelSum=0;}
        if(wheelSum<=-60) {decrement(); wheelSum=0;}
        QLineEdit::wheelEvent(event);
    }

    void mousePressEvent(QMouseEvent * event)
    {
        dragStart=event->pos();
        lastIncrement=0;
        QLineEdit::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent * event)
    {
        if(lastIncrement==0 && isReadOnly())
        {
            setReadOnly(false);
            selectAll();
            QLineEdit::mouseReleaseEvent(event);
        }
    }
    void mouseMoveEvent(QMouseEvent * event)
    {
        if(isReadOnly())
        {
            if(event->buttons()==Qt::LeftButton)
            {
                QPoint dragD=event->pos()-dragStart;
                int newIncrement=(dragD.x()-dragD.y())/dragStep;
                if(newIncrement-lastIncrement>0) increment();
                if(newIncrement-lastIncrement<0) decrement();
                lastIncrement=newIncrement;
            }
        } else
            QLineEdit::mouseMoveEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        if(!qIsNaN(defaultValue))
        {
            setValue(defaultValue);
            finishEditing();
            lastIncrement=1; //to prevent re-editing on mouse release
        }
    }

    bool event(QEvent *event)
    {
        switch(event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchCancel:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate:
        {
            event->accept();
            QTouchEvent* touchEvent=(QTouchEvent*)event;
            if(touchEvent->touchPoints().isEmpty() || touchEvent->type()==QEvent::TouchCancel)
                return true;
            if(touchEvent->touchPoints().first().state()==Qt::TouchPointReleased && touchEvent->touchPoints().first().pos()==touchEvent->touchPoints().first().startPos() && isEnabled())
            {
#if defined(Q_OS_WIN32)
                //from http://sysmagazine.com/posts/163333/
                QDesktopServices::openUrl(QUrl::fromLocalFile(qgetenv("COMMONPROGRAMW6432")+"\\microsoft shared\\ink\\TabTip.exe"));
                usingTouchKeyboard=true;
#endif
            }
            if(touchEvent->touchPoints().first().state()==Qt::TouchPointPressed && touchEvent->touchPoints().count()==1)
                dragStep=touchDragStep;
            if(touchEvent->touchPoints().first().state()==Qt::TouchPointReleased && touchEvent->touchPoints().count()==1)
                dragStep=mouseDragStep;
            return true;
        }
        default:
            return QLineEdit::event(event);;
        }
    }
    void paintEvent(QPaintEvent *event)
    {
        int topMargin=contentsMargins().top();
        int bottomMargin=contentsMargins().bottom();
        int leftMargin=contentsMargins().left();
        int rightMargin=contentsMargins().right();
        int contentsWidth=width()-leftMargin-rightMargin;
        if(progress)
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, false);
            double progressValue=0.;
            if(m_step!=0.)
                progressValue=qBound(0.,(value()-min)/(max-min),1.);
            if(m_count!=0.)
                progressValue=qBound(0.,pow((value()-min)/(max-min),1./m_power),1.);
            if(!m_list.isEmpty())
                progressValue=double(m_list.indexOf(value()))/double(m_list.count()-1);
            painter.fillRect(leftMargin,topMargin,qRound(double(contentsWidth)*progressValue),height()-(topMargin+bottomMargin),palette().color(QPalette::AlternateBase));
        }

        QFontMetrics metrics(font());
        int prefixWidth=metrics.width(prefix)+prefixMargin;
        int textWidth=metrics.width(text());
        int suffixWidth=metrics.width(suffix)+suffixMargin;
        int center=prefixWidth+(contentsWidth-prefixWidth-suffixWidth)/2;
        if(!prefix.isEmpty() || !suffix.isEmpty())
            setTextMargins(prefixWidth,0,suffixWidth,0);

        QLineEdit::paintEvent(event);

        if(!prefix.isEmpty())
        {
            QPainter painter(this);
            painter.setPen(QPen(palette().color(QPalette::Foreground)));
            if(alignment()&(Qt::AlignLeading|Qt::AlignTrailing))
                painter.drawText(QRect(leftMargin,topMargin,prefixWidth,height()-(topMargin+bottomMargin)), Qt::AlignLeading|(alignment()&Qt::AlignVertical_Mask), prefix);
            if(alignment()&Qt::AlignHCenter)
                painter.drawText(QRect(leftMargin+center-textWidth/2-prefixWidth,topMargin,prefixWidth,height()-(topMargin+bottomMargin)), Qt::AlignLeading|(alignment()&Qt::AlignVertical_Mask), prefix);
        }
        if(!suffix.isEmpty())
        {
            QPainter painter(this);
            painter.setPen(QPen(palette().color(QPalette::Foreground)));
            if(alignment()&Qt::AlignLeading)
                painter.drawText(QRect(leftMargin+prefixWidth+textWidth,topMargin,suffixWidth,height()-(topMargin+bottomMargin)), Qt::AlignTrailing|(alignment()&Qt::AlignVertical_Mask), suffix);
            if(alignment()&Qt::AlignHCenter)
                painter.drawText(QRect(leftMargin+center+textWidth/2,topMargin,suffixWidth,height()-(topMargin+bottomMargin)), Qt::AlignTrailing|(alignment()&Qt::AlignVertical_Mask), suffix);
            if(alignment()&Qt::AlignTrailing)
                painter.drawText(QRect(width()-suffixWidth-rightMargin,topMargin,suffixWidth,height()-(topMargin+bottomMargin)), Qt::AlignTrailing|(alignment()&Qt::AlignVertical_Mask), suffix);
        }
    }
private:
    double min, max;
    bool minStrict, maxStrict;
    int decimals;

    double m_step, ctrlStep;
    int m_count, ctrlCount;
    double m_power;
    QList<double> m_list;
    bool listStrict;
    bool incrementable;

    double defaultValue;

    int mouseDragStep, touchDragStep, dragStep;
    QPoint dragStart;
    int lastIncrement;
    int wheelSum;
    bool usingTouchKeyboard;
    IncrementDiff m_incrementDiff;

    bool progress;

    QString prefix, suffix;
    int prefixMargin, suffixMargin;

    QLineEditValidator validator;
};

#endif // QLINEEDITEXT_H
