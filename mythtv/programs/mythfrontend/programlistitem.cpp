#include <qimage.h>
#include <qpixmap.h>
#include <qapplication.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "tv.h"
#include "programlistitem.h"

#include "libmyth/mythcontext.h"

void MyListView::keyPressEvent(QKeyEvent *e)
{
    if (currentItem() && !currentItem()->isEnabled())
    {
    }
    else
    {
        switch(e->key())
        {
            case 'd': case 'D': emit deletePressed(currentItem()); return;
            case 'p': case 'P': emit playPressed(currentItem()); return;
            case Key_Space: emit spacePressed(currentItem()); return;
        }
    }

    QListView::keyPressEvent(e);
}

ProgramListItem::ProgramListItem(MythContext *context, QListView *parent, 
                                 ProgramInfo *lpginfo, int type, TV *ltv, 
                                 QString lprefix)
               : QListViewItem(parent)
{
    prefix = lprefix;
    tv = ltv;
    pginfo = lpginfo;
    pixmap = NULL;
    m_context = context;
  
    QString dateformat = context->GetSetting("DateFormat");
    if (dateformat == "")
        dateformat = "ddd MMMM d";
    QString timeformat = context->GetSetting("TimeFormat");
    if (timeformat == "")
        timeformat = "h:mm AP";
 
    if (type == 0 || type == 1)
    { 
        setText(0, pginfo->startts.toString(dateformat + " " + timeformat));
        setText(1, pginfo->title);

        if (type == 1)
        {
            QString filename = pginfo->GetRecordFilename(prefix);

            struct stat64 st;
 
            long long size = 0;
            if (stat64(filename.ascii(), &st) == 0)
                size = st.st_size;
            long int mbytes = size / 1024 / 1024;
            QString filesize = QString("%1 MB").arg(mbytes);

            setText(2, filesize);
        }
    }
    else
    {
        if (context->GetNumSetting("DisplayChanNum") == 0)
            setText(0, pginfo->chansign);
        else
            setText(0, pginfo->chanstr);
        setText(1, pginfo->startts.toString(dateformat + " " + timeformat));
        setText(2, pginfo->title);
    }
}

QPixmap *ProgramListItem::getPixmap(void)
{
    if (m_context->GetNumSetting("GeneratePreviewPixmaps") != 1)
        return NULL;

    if (pixmap)
        return pixmap;

    QString filename = pginfo->GetRecordFilename(prefix);
    filename += ".png";

    int screenheight = 0, screenwidth = 0;
    float wmult = 0, hmult = 0;

    m_context->GetScreenSettings(screenwidth, wmult, screenheight, hmult);

    pixmap = m_context->LoadScalePixmap(filename, screenwidth, screenheight,
                                        wmult, hmult);

    if (pixmap)
        return pixmap;

    int len = 0;
    int video_width, video_height;
    unsigned char *data = (unsigned char *)tv->GetScreenGrab(pginfo, 65, len,
                                                             video_width,
                                                             video_height);

    if (data)
    {
        QImage img(data, video_width, video_height, 32, NULL, 65536 * 65536, 
                   QImage::LittleEndian);
        img = img.smoothScale(160, 120);

        img.save(filename.ascii(), "PNG");

        delete [] data;
 
        pixmap = m_context->LoadScalePixmap(filename, screenwidth, screenheight,
                                            wmult, hmult);

        if (pixmap)
            return pixmap;
    }

    return NULL;
}

void ProgramListItem::paintCell(QPainter *p, const QColorGroup &cg,
                                int column, int width, int alignment)
{
    QColorGroup _cg(cg);
    QColor c = _cg.text();

    if (!pginfo->recording)
    {
        _cg.setColor(QColorGroup::Text, Qt::gray);
        _cg.setColor(QColorGroup::HighlightedText, Qt::gray);
    }
    else if (pginfo->conflicting)
    {
        _cg.setColor(QColorGroup::Text, Qt::red);
        _cg.setColor(QColorGroup::HighlightedText, Qt::red);
    }

    QListViewItem::paintCell(p, _cg, column, width, alignment);

    _cg.setColor(QColorGroup::Text, c);
}
