#pragma execution_character_set("utf-8")
#include <QApplication>
#include <listview.h>
#include <qmath.h>

ListView::ListView(QWidget* parent)
	: QListView(parent)
{
}

void ListView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	QListView::dataChanged(topLeft, bottomRight, roles);
	//如果數據改變則滾動到底部
	//scrollToBottom();
}

void ListView::setModel(StringListModel* model)
{
	if (model)
	{
		StringListModel* old_mod = (StringListModel*)this->model();
		if (old_mod)
			disconnect(old_mod, &StringListModel::dataAppended, this, &QListView::scrollToBottom);
		QListView::setModel(model);
		connect(model, &StringListModel::dataAppended, this, &QListView::scrollToBottom);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
StringListModel::StringListModel(QObject* parent)
	: QAbstractListModel(parent)
{}


void StringListModel::append(const QString& str)
{
	QWriteLocker locker(&m_stringlistLocker);
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	if (m_list.size() >= 512)
	{
		m_list.clear();
		//listCount = 0;
	}
	m_list.append(str);
	endInsertRows();
	emit dataChanged(index(rowCount() - 1), index(rowCount() - 1), QVector<int>() << Qt::DisplayRole);
	emit dataAppended();
}

void StringListModel::append(const QStringList& strs)
{
	QWriteLocker locker(&m_stringlistLocker);
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	if (m_list.size() >= 512)
	{
		m_list.clear();
		//listCount = 0;
	}
	m_list.append(strs.toVector());
	endInsertRows();
	emit dataChanged(index(rowCount() - 1), index(rowCount() - 1), QVector<int>() << Qt::DisplayRole);
	emit dataAppended();
}

QString StringListModel::takeFirst()
{
	QWriteLocker locker(&m_stringlistLocker);
	if (m_list.size() > 0)
	{
		beginRemoveRows(QModelIndex(), 0, 0);
		QString str = m_list.takeFirst();
		endRemoveRows();
		return str;
	}
	return QString();
}

void StringListModel::remove(const QString& str)
{
	QWriteLocker locker(&m_stringlistLocker);
	int index = m_list.indexOf(str);
	if (index != -1)
	{
		beginRemoveRows(QModelIndex(), index, index);
		m_list.removeAt(index);
		endRemoveRows();
	}
}

void StringListModel::setStringList(const QStringList& list)
{
	QWriteLocker locker(&m_stringlistLocker);
	beginResetModel();
	m_list = list.toVector();
	endResetModel();
	emit dataAppended();
}

void StringListModel::clear()
{
	QWriteLocker locker(&m_stringlistLocker);
	beginResetModel();
	m_list.clear();
	//listCount = 0;
	endResetModel();
}

void StringListModel::sort(int column, Qt::SortOrder order)
{
	Q_UNUSED(column);
	QWriteLocker locker(&m_stringlistLocker);
	beginResetModel();
	if (order == Qt::AscendingOrder)
	{
#if _MSVC_LANG > 201703L
		std::ranges::sort(m_list);
#else
		std::sort(m_list.begin(), m_list.end());
#endif
	}
	else
	{
#if _MSVC_LANG > 201703L
		std::ranges::sort(m_list, std::greater<QString>());
#else
		std::sort(m_list.begin(), m_list.end(), std::greater<QString>());
#endif
	}
	endResetModel();
}

bool StringListModel::insertRows(int row, int count, const QModelIndex& parent)
{
	Q_UNUSED(parent);
	beginInsertRows(QModelIndex(), row, row + count - 1);
	endInsertRows();
	return true;
}
bool StringListModel::removeRows(int row, int count, const QModelIndex& parent)
{
	Q_UNUSED(parent);
	beginRemoveRows(QModelIndex(), row, row + count - 1);
	endRemoveRows();
	return true;
}
bool StringListModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
	Q_UNUSED(destinationParent);
	Q_UNUSED(sourceParent);
	beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild);
	endMoveRows();
	return true;
}

QMap<int, QVariant> StringListModel::itemData(const QModelIndex& index) const
{
	QMap<int, QVariant> map;
	map.insert(Qt::DisplayRole, m_list.at(index.row()));
	return map;
}

QVariant StringListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();


	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (index.row() >= m_list.size() || index.row() < 0)
			return QVariant();
		return m_list.at(index.row());
	}
	case Qt::BackgroundRole:
	{
		static const QBrush brushBase = qApp->palette().base();
		static const QBrush brushAlternate = qApp->palette().alternateBase();
		return ((index.row() & 99) == 0) ? brushBase : brushAlternate;
		/*int batch = (index.row() / 100) % 2;
		if (batch == 0)
			return brushBase;
		else
			return brushAlternate;*/
	}
	case Qt::ForegroundRole:
	{

		static const QRegularExpression rexError(R"(((?i)\[error\]|\[錯誤\]|\[错误\]))");
		static const QRegularExpression rexFatal(R"(((?i)\[fatal\]|\[異常\]|\[异常\]))");
		static const QRegularExpression rexWarn(R"(((?i)\[warn\]|\[警告\]|\[警告\]))");
		static const QRegularExpression rexInfo(R"(((?i)\[info\]|\[資訊\]|\[资讯\]))");
		static const QBrush colorError(QColor(255, 128, 128));
		static const QBrush colorFatal(QColor(168, 46, 46));
		static const QBrush colorWarn(QColor(206, 145, 120));
		static const QBrush colorInfo(QColor(181, 206, 168));
		static const QBrush colorOther(QColor(212, 212, 212));
		if (index.row() >= m_list.size() || index.row() < 0)
			return colorOther;
		QString data = m_list.at(index.row());
		if (data.contains(rexError))
		{
			return colorError;
		}
		else if (data.contains(rexFatal))
		{
			return colorFatal;
		}
		else if (data.contains(rexWarn))
		{
			return colorWarn;
		}
		else if (data.contains(rexInfo))
		{
			return colorInfo;
		}
		else
			return colorOther;
	}
	default: break;
	}
	return QVariant();
}

bool StringListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role == Qt::EditRole)
	{
		if (index.row() >= 0 && index.row() < m_list.size())
		{
			m_list.replace(index.row(), value.toString());
			emit dataChanged(index, index, QVector<int>() << role);
			return true;
		}
	}
	return false;
}

void StringListModel::swapRowUp(int source)
{
	if (source > 0)
	{
		QWriteLocker locker(&m_stringlistLocker);
		beginMoveRows(QModelIndex(), source, source, QModelIndex(), source - 1);
		m_list.swapItemsAt(source, source - 1);
		endMoveRows();
	}
}
void StringListModel::swapRowDown(int source)
{
	if (source + 1 < m_list.size())
	{
		QWriteLocker locker(&m_stringlistLocker);
		beginMoveRows(QModelIndex(), source, source, QModelIndex(), source + 2);
		m_list.swapItemsAt(source, source + 1);
		endMoveRows();
	}
}