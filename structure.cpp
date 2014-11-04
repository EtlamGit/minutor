#include "structure.h"
#include "nbt.h"

#include <math.h>
#include <assert.h> 

Structure::Structure( int nx1, int ny1, int nz1,
			          int nx2, int ny2, int nz2,
			          QString nid )
  :	id(nid)
  ,	x1(nx1)
  ,	y1(ny1)
  ,	z1(nz1)
  ,	x2(nx2)
  ,	y2(ny2)
  ,	z2(nz2)
{
	assert(nx2 >= nx1);
	assert(ny2 >= ny1);
	assert(nz2 >= nz1);

	// generate color from hased structure name	
	quint32 hue = qHash(this->id);
	color.setHsv(hue % 360, 255, 255, 64);
}


Structure::Structure( int nx1, int ny1, int nz1,
			          int nx2, int ny2, int nz2,
			          QString  nid,
					  QVariant nproperties)
  :	id(nid)
  ,	x1(nx1)
  ,	y1(ny1)
  ,	z1(nz1)
  ,	x2(nx2)
  ,	y2(ny2)
  ,	z2(nz2)
  , properties(nproperties)
{
	assert(nx2 >= nx1);
	assert(ny2 >= ny1);
	assert(nz2 >= nz1);

	// generate color from hased structure name	
	quint32 hue = qHash(this->id);
	color.setHsv(hue % 360, 255, 255, 64);
}


bool Structure::load( const QVariant& feature )
{
	if ((QMetaType::Type)feature.type() == QMetaType::QVariantMap)
	{
		QMap<QString, QVariant> featureProperties = feature.toMap();
		//check for required properties
		if (   featureProperties.contains("BB") //bounding box... gives us the position
		    && (QMetaType::Type)featureProperties["BB"].type() == QMetaType::QVariantList
		    && featureProperties.contains("id") //name of the feature type
		    )
		{
			QList<QVariant> bb = featureProperties["BB"].toList();
			if (bb.size() == 6)
			{
				id = featureProperties["id"].toString();
			    x1 = bb[0].toInt();
			    y1 = bb[1].toInt();
			    z1 = bb[2].toInt();
			    x2 = bb[3].toInt();
			    y2 = bb[4].toInt();
			    z2 = bb[5].toInt();
				properties = featureProperties;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	// generate color from hased structure name	
	quint32 hue = qHash(this->id);
	color.setHsv(hue % 360, 255, 255, 64);
	
	return true;
}


bool Structure::intersects(double x1, double y1, double z1,
                           double x2, double y2, double z2) const
{
    return ((x1 <= this->x2) && (this->x1 <= x2)) &&
           ((y1 <= this->y2) && (this->y1 <= y2)) &&
           ((z1 <= this->z2) && (this->z1 <= z2));
}


void Structure::draw(double offsetX, double offsetZ, double scale, QPainter& canvas) const
{    
	double left = ((x1 - offsetX) * scale) + MIN_SIZE/4;
	double top = ((z1 - offsetZ) * scale) + MIN_SIZE/4;
	double w = ((x2 - x1 + 1) * scale) - MIN_SIZE/2;
	double h = ((z2 - z1 + 1) * scale) - MIN_SIZE/2;
	if (w < MIN_SIZE)
	{
		left = (left + w / 2.0) - MIN_SIZE / 2.0;
		w = MIN_SIZE;
	}
	if (h < MIN_SIZE)
	{
		top = (top + h / 2.0) - MIN_SIZE / 2.0;
		h = MIN_SIZE;
	}
	QPen pen = canvas.pen();
	pen.setColor(color);
	//pen.setWidth(MIN_SIZE/2);
	canvas.setPen(QColor(0, 0, 0, 0));
	canvas.setBrush(QBrush(color));
	canvas.drawRoundedRect(QRect(left, top, w, h), MIN_SIZE, MIN_SIZE);
	QSize labelsize = canvas.fontMetrics().size(0, id);
	canvas.setPen(Qt::black);
	if (labelsize.height() < h && labelsize.width() < w)
	{
		canvas.drawText(left, top, w, h, Qt::AlignCenter, id);
	}
}
